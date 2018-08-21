//
//  CocoaInput.swift
//  MacMAME
//
//  Created by C.W. Betts on 8/20/18.
//

import Cocoa
import IOKit.hid
import HIDUtilities

struct t_hidJoyDevice {
	var device: IOHIDDevice
	var	axisElements: [IOHIDElement]
	var axisElementValue: [Int]
	var buttonElements: [IOHIDElement]
	var buttonElementValue: [Int]
	var povElements: [IOHIDElement]
	var povElementValue: [Int]
}


class CocoaInput: NSObject {

}
