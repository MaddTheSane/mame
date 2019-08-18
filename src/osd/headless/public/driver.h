#import <Foundation/Foundation.h>

@interface Drivers: NSObject
- (void)listXML:(FILE *)out dtd:(BOOL)dtd patterns:(NSArray<NSString *> *)patterns;
@end

@interface GameDriver: NSObject
@end