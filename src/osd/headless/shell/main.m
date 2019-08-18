#import <Foundation/Foundation.h>

#import <dlfcn.h>
#import <objc/runtime.h>
#import <osd.h>
#import <driver.h>

#include "stdio.h"

@interface MyDelegate: NSObject<OSDDelegate>
@end

@implementation MyDelegate
{
	OSD *_osd;
}
- (instancetype)initWithOSD:(OSD *)osd {
	self = [super init];
	
	_osd = osd;
	
	return self;
}

- (void)willInitializeWithBounds:(NSSize)bounds fps:(float)fps aspect:(NSSize)aspect
{
	printf("initializing\n");
	InputDevice *joy = [_osd.joystick addDeviceNamed:@"Joy 1"];
}

- (void)updateAudioBuffer:(int16_t const *)buffer samples:(NSInteger)samples
{

}

- (void)logLevel:(OSDLogLevel)level format:(NSString *)fmt args:(va_list)args
{
	NSLogv(fmt, args);
}
@end

int main(int argc, char *argv[])
{
#if 0
	char const * path = "cmake-build-headless-dbg/build/projects/headless/mametiny/cmake/mametiny/libmametiny_headless.dylib";
#else
	char const * path = "mamearcade_headless.dylib";
#endif
	void *handle = dlopen(path, RTLD_LAZY);
	if (handle == nil)
	{
		printf("no library: %s\n", dlerror());
		return 1;
	}
	
	Class driversCLASS = NSClassFromString(@"Drivers");
	
	Drivers *d = (Drivers *)[driversCLASS new];
	[d listXML:stdout dtd:YES patterns:@[@"gold*"]];
	
//	char *buf;
//	size_t len;
//
//	FILE *out = open_memstream(&buf, &len);
//	[d listXML:out dtd:YES patterns:nil];
//	fclose(out);
//
//	NSString *res = [[NSString alloc] initWithBytesNoCopy:buf length:len encoding:NSUTF8StringEncoding freeWhenDone:YES];
//	puts(res.UTF8String);
//	res = nil;
	
	@try
	{
		Class osdCLASS = NSClassFromString(@"OSD");
		
		OSD *shared = [osdCLASS shared];
		shared.delegate = [[MyDelegate alloc] initWithOSD:shared];
		shared.verboseOutput = NO;
		[shared setBasePath:@"/Volumes/Data/games/mame"];
		[shared setBuffer:malloc(2048*2048*4) size:NSMakeSize(2048, 2048)];
		
		BOOL res = [shared loadGame:@"targ" error:nil];
		if (!res)
		{
			printf("game not found\n");
		}
		
		printf("supports save: %s\n", shared.supportsSave ? "Y" : "N");
		
		for (int i = 0; i < 50; i++) {
			[shared execute];
		}
	} @finally
	{
		dlclose(handle);
	}
	
	return 0;
}
