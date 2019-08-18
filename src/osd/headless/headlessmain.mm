#import <Foundation/Foundation.h>

// standard includes
#include <unistd.h>
#include <algorithm>

// MAME headers
#include "emu.h"
#include "../../frontend/mame/audit.h"
#include "osdepend.h"
#include "emuopts.h"
#include "../../frontend/mame/ui/menuitem.h"
#include "validity.h"
#include "render.h"
#include "ui/uimain.h"
#include <inputdev.h>

// OSD headers
#include "../modules/osdhelper.h"
#include "public/osd.h"
#include "modules/lib/osdlib.h"
#import "../../../frontend/mame/mame.h"

// Renderer headers
#include "rendersw.hxx"

NSString *const MAMEErrorDomain = @"org.openemu.mame.ErrorDomain";

void osd_setup_osd_specific_emu_options(emu_options &opts)
{
	[OSD class];
}

#pragma mark - definition

class headless_osd_interface : public osd_interface, osd_output
{
public:
	enum osd_state
	{
		uninitialized,
		initialized,
		error,
	};
	
	explicit headless_osd_interface(emu_options &options);
	~headless_osd_interface() final;
	
	// current state
	osd_state state() const { return m_state; };
	
	// general overridables
	void init(running_machine &machine) override;
	void exit();
	void update(bool skip_redraw) override;
	
	// debugger overridables
	void init_debugger() override
	{};
	
	void wait_for_debugger(device_t &device, bool firststop) override
	{};
	
	// audio overridables
	void update_audio_stream(const int16_t *buffer, int samples_this_frame) override;
	
	void set_mastervolume(int attenuation) override
	{};
	
	bool no_sound() override
	{ return false; };
	
	// input overridables
	void customize_input_type_list(simple_list<input_type_entry> &typelist) override;
	
	// video overridables
	void add_audio_to_recording(const int16_t *buffer, int samples_this_frame) override
	{};
	
	std::vector<ui::menu_item> get_slider_list() override
	{ return m_sliders; };
	
	void *buffer() const
	{ return m_buffer; }
	
	osd_dim buffer_size() const
	{ return m_buffer_size; }
	
	void set_buffer(void *buffer, osd_dim size);
	
	// font interface
	osd_font::ptr font_alloc() override;
	bool
	get_font_families(std::string const &font_path, std::vector<std::pair<std::string, std::string> > &result) override;
	
	// command option overrides
	bool execute_command(const char *command) override
	{ return false; }
	
	// midi interface
	osd_midi_device *create_midi_device() override
	{ return nullptr; };
	
	// osd_output interface ...
	void output_callback(osd_output_channel channel, const char *msg, va_list args) override;
	
	bool verbose() const
	{ return m_print_verbose; }
	
	void set_verbose(bool print_verbose) override
	{ m_print_verbose = print_verbose; }
	
	// other
	void set_delegate(id <OSDDelegate> delegate)
	{ m_delegate = delegate; }

protected:
	
	// internal state
	running_machine *m_machine;
	emu_options &m_options;
	osd_state m_state;
	
	// ui
	std::vector<ui::menu_item> m_sliders;
	bool m_print_verbose;
	
	// video
	double m_fps;
	render_target *m_target;
	void *m_buffer;
	osd_dim m_buffer_size;
	osd_dim m_target_size;
	
	// delegate
	id <OSDDelegate> m_delegate;
};

#pragma mark - Other categories

@interface InputClass ()
- (instancetype)initWithClass:(input_class *)clas;
@end

#pragma mark - OSD Objective C Interface

#pragma clang diagnostic ignored "-Wenum-compare"

static_assert(InputItemID::InputItemID_ABSOLUTE_MAXIMUM == input_item_id::ITEM_ID_ABSOLUTE_MAXIMUM,
              "enum is different");

@implementation OSD
{
	emu_options *_options;
	headless_osd_interface *_osd;
	mame_machine_manager *_manager;
	machine_config *_config;
	running_machine *_machine;
	BOOL _supportsSave;
	NSString *_basePath;
	
	InputClass *_joystick;
	InputClass *_mouse;
	InputClass *_keyboard;
	
	
	bool _isEmpty;
}

- (instancetype)init
{
	self = [super init];
	if (self == nil)
	{
		return nil;
	}
	
	_options = global_alloc(emu_options);
	_options->set_value(OPTION_PLUGINS, 0, OPTION_PRIORITY_HIGH); // disable LUA plugins
	_options->set_value(OPTION_READCONFIG, 0, OPTION_PRIORITY_HIGH); // disable reading .ini files
	_options->set_value(OPTION_SAMPLERATE, 48000, OPTION_PRIORITY_HIGH);
	// disable throttling as OpenEmu handles frame pacing
	_options->set_value(OPTION_THROTTLE, 0, OPTION_PRIORITY_HIGH);
	_options->set_value(OPTION_CHEAT, 1, OPTION_PRIORITY_HIGH);
	_options->set_value(OPTION_SKIP_GAMEINFO, 1, OPTION_PRIORITY_HIGH);
	
	_osd = global_alloc(headless_osd_interface(*_options));
	
	_manager = mame_machine_manager::instance(*_options, *_osd);
	_manager->start_http_server();
	_manager->start_luaengine();
	
	// arbitrary size
	_maxBufferSize = NSMakeSize(2048, 2048);
	
	return self;
}

- (void)dealloc
{
	[self _freeMachine];
	global_free(_manager);
	global_free(_osd);
	global_free(_options);
}

- (InputClass *)joystick
{
	if (_joystick == nil)
	{
		_joystick = [[InputClass alloc] initWithClass:&_machine->input().device_class(DEVICE_CLASS_JOYSTICK)];
	}
	return _joystick;
}

- (InputClass *)mouse
{
	if (_mouse == nil)
	{
		_mouse = [[InputClass alloc] initWithClass:&_machine->input().device_class(DEVICE_CLASS_MOUSE)];
	}
	return _mouse;
}

- (InputClass *)keyboard
{
	if (_keyboard == nil)
	{
		_keyboard = [[InputClass alloc] initWithClass:&_machine->input().device_class(DEVICE_CLASS_KEYBOARD)];
	}
	return _keyboard;
}

- (void)setDelegate:(id <OSDDelegate>)delegate
{
	_delegate = delegate;
	_osd->set_delegate(delegate);
}

- (void)setVerboseOutput:(BOOL)val
{
	_osd->set_verbose(val);
}

- (BOOL)verboseOutput
{
	return (BOOL) _osd->verbose();
}

#pragma mark - paths

#define OPTION_PROPERTY(OPT, SET, GET) \
- (void)set##SET##Path:(NSString *)path \
{ \
    _options->set_value(OPTION_ ## OPT, path.UTF8String, OPTION_PRIORITY_HIGH); \
} \
\
- (NSString *)GET##Path \
{ \
    return [NSString stringWithUTF8String:_options->value(OPTION_ ## OPT)]; \
} \


OPTION_PROPERTY(MEDIAPATH, Roms, roms)

OPTION_PROPERTY(HASHPATH, Hash, hash)

OPTION_PROPERTY(SAMPLEPATH, Samples, samples)

OPTION_PROPERTY(ARTPATH, Art, art)

OPTION_PROPERTY(CTRLRPATH, Controller, controller)

OPTION_PROPERTY(CHEATPATH, Cheat, cheat)

OPTION_PROPERTY(CROSSHAIRPATH, CrossHair, crossHair)

OPTION_PROPERTY(PLUGINSPATH, Plugins, plugins)

OPTION_PROPERTY(LANGUAGEPATH, Language, language)

#undef OPTION_PROPERTY

- (void)setBasePath:(NSString *)path
{
	_basePath = [path copy];
	
	_options->set_value(OPTION_MEDIAPATH, [NSString pathWithComponents:@[path, @"roms"]].UTF8String,
	                    OPTION_PRIORITY_HIGH);
	_options->set_value(OPTION_HASHPATH, [NSString pathWithComponents:@[path, @"hash"]].UTF8String,
	                    OPTION_PRIORITY_HIGH);
	_options->set_value(OPTION_SAMPLEPATH, [NSString pathWithComponents:@[path, @"samples"]].UTF8String,
	                    OPTION_PRIORITY_HIGH);
	_options->set_value(OPTION_ARTPATH, [NSString pathWithComponents:@[path, @"artwork"]].UTF8String,
	                    OPTION_PRIORITY_HIGH);
	_options->set_value(OPTION_CTRLRPATH, [NSString pathWithComponents:@[path, @"ctrlr"]].UTF8String,
	                    OPTION_PRIORITY_HIGH);
	_options->set_value(OPTION_CHEATPATH, [NSString pathWithComponents:@[path, @"cheat"]].UTF8String,
	                    OPTION_PRIORITY_HIGH);
	_options->set_value(OPTION_CROSSHAIRPATH, [NSString pathWithComponents:@[path, @"crosshair"]].UTF8String,
	                    OPTION_PRIORITY_HIGH);
	_options->set_value(OPTION_PLUGINSPATH, [NSString pathWithComponents:@[path, @"plugins"]].UTF8String,
	                    OPTION_PRIORITY_HIGH);
	_options->set_value(OPTION_LANGUAGEPATH, [NSString pathWithComponents:@[path, @"language"]].UTF8String,
	                    OPTION_PRIORITY_HIGH);
	
	_options->set_value(OPTION_NVRAM_DIRECTORY, [NSString pathWithComponents:@[path, @"nvram"]].UTF8String,
	                    OPTION_PRIORITY_HIGH);
	_options->set_value(OPTION_CFG_DIRECTORY, [NSString pathWithComponents:@[path, @"cfg"]].UTF8String,
	                    OPTION_PRIORITY_HIGH);
}

- (void)setBuffer:(void *)buffer size:(NSSize)size
{
	auto dim = osd_dim(static_cast<int>(size.width), static_cast<int>(size.height));
	_osd->set_buffer(buffer, dim);
}

- (BOOL)loadGame:(NSString *)name error:(NSError **)error
{
	_driverName = name;
	
	driver_enumerator drivlist(*_options, _driverName.UTF8String);
	media_auditor auditor(drivlist);
	
	while (drivlist.next())
	{
		media_auditor::summary summary = auditor.audit_media(AUDIT_VALIDATE_FAST);
		if (summary == media_auditor::CORRECT || summary == media_auditor::BEST_AVAILABLE)
		{
			break;
		}
		
		std::ostringstream output;
		auditor.summarize(drivlist.driver().name, &output);
		if (error != nil)
		{
			NSString *auditOutput = @(output.str().c_str());
			NSDictionary<NSErrorUserInfoKey, id> *info = @{
					NSLocalizedDescriptionKey: NSLocalizedString(@"Audit failed", "audit failed"),
					NSLocalizedFailureReasonErrorKey: auditOutput,
			};
			*error = [NSError errorWithDomain:MAMEErrorDomain code:MAMEErrorAuditFailed userInfo:info];
		}

		osd_printf_error("audit failed: %s", output.str().c_str());
		
		return NO;
	}
	
	auto proposed_system = &driver_list::driver(drivlist.current());
	if (proposed_system->flags & (MACHINE_CLICKABLE_ARTWORK | MACHINE_REQUIRES_ARTWORK))
	{
		NSDictionary<NSErrorUserInfoKey, id> *info = @{
				NSLocalizedDescriptionKey: NSLocalizedString(@"Systems which require artwork to operate are not supported", "System not supported"),
		};
		*error = [NSError errorWithDomain:MAMEErrorDomain code:MAMEErrorUnsupportedROM userInfo:info];
		return NO;
	}
	
	if (proposed_system->flags & MACHINE_MECHANICAL)
	{
		NSDictionary<NSErrorUserInfoKey, id> *info = @{
				NSLocalizedDescriptionKey: NSLocalizedString(@"Mechanical systems are not supported", "System not supported"),
		};
		*error = [NSError errorWithDomain:MAMEErrorDomain code:MAMEErrorUnsupportedROM userInfo:info];
		return NO;
	}
	
	try
	{
		_options->set_system_name(name.UTF8String);
	}
	catch (options_error_exception &ex)
	{
		// NOTE(sgc): this should never happen, given we've validated the system
		return NO;
	}
	
	return [self _initializeGame:error];
}

- (void)unload
{
	_osd->set_buffer(nullptr, osd_dim());
	[self _freeMachine];
}

#pragma mark - save / load

- (BOOL)loadStateFromFileAtPath:(NSString *)fileName error:(NSError **)error
{
	if (_supportsSave)
	{
		std::string name(fileName.UTF8String);
		_machine->immediate_load(name.c_str());
		return YES;
	}
	return NO;
}

- (BOOL)saveStateFromFileAtPath:(NSString *)fileName error:(NSError **)error
{
	if (_supportsSave)
	{
		std::string name(fileName.UTF8String);
		_machine->immediate_save(name.c_str());
		return YES;
	}
	return NO;
}


#pragma mark - serialization

- (NSData *)serializeState
{
	if (!_supportsSave)
	{
		return nil;
	}
	
	auto sz = static_cast<size_t>(_machine->save().state_size());
	void *buf = malloc(sz);
	auto res = _machine->save().write_data(buf, sz);
	if (res != STATERR_NONE)
	{
		free(buf);
		return nil;
	}
	
	return [[NSData alloc] initWithBytesNoCopy:buf length:sz freeWhenDone:YES];
}

- (BOOL)deserializeState:(NSData *)data
{
	if (!_supportsSave)
	{
		return NO;
	}
	
	auto res = _machine->save().read_data(const_cast<void *>(data.bytes), data.length);
	return res == STATERR_NONE;
}


- (void)_freeMachine
{
	_joystick = nil;
	_mouse = nil;
	_keyboard = nil;
	_supportsSave = NO;
	
	_manager->set_machine(nullptr);
	
	if (_machine)
	{
		_machine->headless_deinit();
		global_free(_machine);
		_machine = nil;
	}
	
	if (_config)
	{
		global_free(_config);
		_config = nil;
	}
}

- (BOOL)_initializeGame:(NSError **)error
{
	[self _freeMachine];
	
	_manager->clear_new_driver_pending();
	
	// if no driver, use the internal empty driver
	const game_driver *system = _options->system();
	if (system == nullptr)
	{
		system = &GAME_NAME(___empty);
	}
	
	// otherwise, perform validity checks before anything else
	_isEmpty = (system == &GAME_NAME(___empty));
	if (!_isEmpty)
	{
		validity_checker valid(*_options);
		valid.set_verbose(false);
		valid.check_shared_source(*system);
	}
	
	_config = global_alloc(machine_config(*system, *_options));
	_machine = global_alloc(running_machine(*_config, *_manager));
	_manager->set_machine(_machine);
	
	_machine->headless_init(_isEmpty);
	if (_osd->state() == headless_osd_interface::osd_state::error)
	{
		[self _freeMachine];
		
		if (error != nil)
		{
			NSDictionary<NSErrorUserInfoKey, id> *info = @{
					NSLocalizedDescriptionKey: NSLocalizedString(@"Failed to initialize; unsupported ROM",
					                                             @"init failed")
			};
			*error = [NSError errorWithDomain:MAMEErrorDomain code:MAMEErrorUnsupportedROM userInfo:info];
		}
		
		return NO;
	}
	
	_supportsSave = (_machine->system().flags & MACHINE_SUPPORTS_SAVE) != 0;
	
	return _machine->phase() == machine_phase::RUNNING;
}

#pragma mark - Execution

- (BOOL)execute
{
	if (_manager->new_driver_pending())
	{
		_manager->commit_new_driver();
		if (![self _initializeGame:nil])
		{
			return NO;
		};
	}
	
	_machine->headless_run();
	
	return YES;
}

- (void)scheduleSoftReset
{
	_machine->schedule_soft_reset();
}

- (void)scheduleHardReset
{
	[self unload];
	[self _initializeGame:nil];
}

+ (OSD *)shared
{
	static OSD *shared;
	
	static dispatch_once_t once;
	dispatch_once(&once, ^{
		shared = [[OSD alloc] init];
	});
	
	return shared;
}

@end

#define NSSTRING_NO_COPY(x) [[NSString alloc] initWithBytesNoCopy:(void *)x length:strlen(x) encoding:NSUTF8StringEncoding freeWhenDone:NO];

@implementation InputDeviceItem
@end

@implementation InputDevice
{
	input_device *_device;
}

- (instancetype)initWithDevice:(input_device *)device
{
	if (!(self = [super init]))
	{
		return nil;
	}
	
	_device = device;
	
	return self;
}

- (NSUInteger)index
{
	return static_cast<NSUInteger>(_device->devindex());
}

- (NSString *)name
{
	return NSSTRING_NO_COPY(_device->name());
}

- (NSString *)id
{
	return NSSTRING_NO_COPY(_device->id());
}

- (InputItemID)addItemNamed:(NSString *)name id:(InputItemID)iid getter:(ItemGetStateFunc)getter context:(void *)context
{
	return static_cast<InputItemID>(_device->add_item(name.UTF8String, static_cast<input_item_id>(iid),
	                                                  reinterpret_cast<item_get_state_func>(getter), context));
}

@end

@implementation InputClass
{
	input_class *_class;
	NSMutableArray<InputDevice *> *_devices;
}

- (instancetype)initWithClass:(input_class *)clas
{
	if (!(self = [super init]))
	{
		return nil;
	}
	
	_class = clas;
	_devices = [[NSMutableArray alloc] initWithCapacity:DEVICE_INDEX_MAXIMUM];
	for (NSUInteger i = 0; i < DEVICE_INDEX_MAXIMUM; i++)
	{
		auto device = _class->device(static_cast<int>(i));
		if (device != nullptr)
		{
			_devices[i] = [[InputDevice alloc] initWithDevice:device];
		}
	}
	
	return self;
}

- (InputDevice *)addDeviceNamed:(NSString *)name
{
	auto device = _class->add_device(name.UTF8String, name.UTF8String);
	auto dev = [[InputDevice alloc] initWithDevice:device];
	_devices[static_cast<NSUInteger>(device->devindex())] = dev;
	return dev;
}

- (InputDevice *)deviceForIndex:(NSUInteger)index
{
	return _devices[index];
}

@end

#pragma mark - headless_osd_interface implementation

#pragma mark - construction / destruction

headless_osd_interface::headless_osd_interface(emu_options &options)
		: m_machine(nullptr), m_options(options), m_state(uninitialized), m_print_verbose(false),
		  m_fps(60.0), m_target(nullptr), m_buffer(nullptr)
{
	osd_output::push(this);
}

headless_osd_interface::~headless_osd_interface()
{
	osd_output::pop(this);
}

#pragma mark - general overridables

void headless_osd_interface::init(running_machine &machine)
{
	screen_device_iterator iter(machine.root_device());
	auto first_screen = iter.first();
	if (!first_screen)
	{
		m_state = osd_state::error;
		return;
	}
	
	m_state = osd_state::initialized;
	m_machine = &machine;
	m_target = m_machine->render().target_alloc();
	
	m_fps = ATTOSECONDS_TO_HZ(first_screen->refresh_attoseconds());
	auto aspect = first_screen->physical_aspect();
	bool rotated = (first_screen->orientation() & ORIENTATION_SWAP_XY) == ORIENTATION_SWAP_XY;
	if (rotated)
	{
		std::swap(aspect.first, aspect.second);
	}
	auto pixel_aspect = (float) aspect.first / aspect.second;
	pixel_aspect = 1.0;
	
	m_target->set_scale_mode(SCALE_INTEGER);
	s32 width, height;
	m_target->compute_minimum_size(width, height);
	m_target_size = osd_dim(width, height);
	m_target->set_bounds(width, height, pixel_aspect);
	
	// ensure we get called on the way out
	machine.add_notifier(MACHINE_NOTIFY_EXIT, machine_notify_delegate(&headless_osd_interface::exit, this));
	
	[m_delegate willInitializeWithBounds:NSMakeSize(width, height) fps:static_cast<float>(m_fps) aspect:NSMakeSize(
			aspect.first, aspect.second)];
}

void headless_osd_interface::exit()
{
	m_state = osd_state::uninitialized;
	m_machine->render().target_free(m_target);
}

void headless_osd_interface::update(bool skip_redraw)
{
	if (!skip_redraw && m_buffer != nullptr)
	{
		auto &prim = m_target->get_primitives();
		u32 width = static_cast<u32>(m_buffer_size.width());
		u32 height = static_cast<u32>(m_buffer_size.height());
		
		prim.acquire_lock();
		software_renderer<uint32_t, 0, 0, 0, 16, 8, 0>::draw_primitives(prim, m_buffer, width, height, width);
		prim.release_lock();
	}
}

#pragma mark - audio overridables

void headless_osd_interface::update_audio_stream(const int16_t *buffer, int samples_this_frame)
{
	[m_delegate updateAudioBuffer:buffer samples:samples_this_frame];
}

#pragma mark - input overridables

void headless_osd_interface::customize_input_type_list(simple_list<input_type_entry> &typelist)
{
	// This function is called on startup, before reading the
	// configuration from disk. Scan the list, and change the
	// default control mappings you want. It is quite possible
	// you won't need to change a thing.
	
	// loop over the defaults
	for (input_type_entry &entry : typelist)
	{
		switch (entry.type())
		{
			// Select + X => UI_CONFIGURE (Menu)
//			case IPT_UI_CONFIGURE:
//				entry.defseq(SEQ_TYPE_STANDARD).set(KEYCODE_TAB, input_seq::or_code, JOYCODE_SELECT, JOYCODE_BUTTON3);
//				break;
//
//				// Select + Start => CANCEL
//			case IPT_UI_CANCEL:
//				entry.defseq(SEQ_TYPE_STANDARD).set(KEYCODE_ESC, input_seq::or_code, JOYCODE_SELECT, JOYCODE_START);
//				break;
			
			// leave everything else alone
			default:
				break;
		}
	}
}

#pragma mark - video overridables

void headless_osd_interface::set_buffer(void *buffer, osd_dim size)
{
	m_buffer = buffer;
	m_buffer_size = size;
}

#pragma mark - font interface

osd_font::ptr headless_osd_interface::font_alloc()
{
	return nullptr;
}

bool headless_osd_interface::get_font_families(std::string const &font_path,
                                               std::vector<std::pair<std::string, std::string> > &result)
{
	return false;
}

#pragma mark - output

void headless_osd_interface::output_callback(osd_output_channel channel, const char *msg, va_list args)
{
	static OSDLogLevel levels[OSD_OUTPUT_CHANNEL_COUNT] = {
			[OSD_OUTPUT_CHANNEL_ERROR] = OSDLogLevelError,
			[OSD_OUTPUT_CHANNEL_WARNING] = OSDLogLevelWarning,
			[OSD_OUTPUT_CHANNEL_INFO] = OSDLogLevelInfo,
			[OSD_OUTPUT_CHANNEL_DEBUG] = OSDLogLevelDebug,
			[OSD_OUTPUT_CHANNEL_VERBOSE] = OSDLogLevelVerbose,
			[OSD_OUTPUT_CHANNEL_LOG] = OSDLogLevelLog,
	};
	
	if (channel == OSD_OUTPUT_CHANNEL_VERBOSE && !m_print_verbose)
	{
		return;
	}
	
	size_t len = strlen(msg) - 1;
	while (isspace(msg[len]) != 0)
	{
		len--;
	}
	len++;
	
	NSString *fmt = [[NSString alloc] initWithBytesNoCopy:const_cast<char *>(msg)
	                                               length:len
	                                             encoding:NSUTF8StringEncoding
	                                         freeWhenDone:NO];
	[m_delegate logLevel:levels[channel] format:fmt args:args];
}
