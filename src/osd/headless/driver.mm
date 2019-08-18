#import "public/driver.h"
#import <emu.h>
#import "../../frontend/mame/info.h"

@implementation Drivers

- (void)listXML:(FILE *)out dtd:(BOOL)dtd patterns:(NSArray<NSString *> *)patterns
{
	std::vector<std::string> args;
	args.reserve(patterns.count);
	for (NSString *p in patterns)
	{
		args.emplace_back(p.UTF8String);
	}
	
	emu_options options;
	info_xml_creator creator(options, dtd);
	creator.output(out, args);
	fflush(out);
}

@end

@implementation GameDriver {
	game_driver *_driver;
}

- (void)a {

}

@end