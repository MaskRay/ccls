#import <Foundation/Foundation.h>

@interface AClass: NSObject
  - (void)anInstanceMethod;
  @property (nonatomic) int aProp;
  @property (readonly) int readOnlyProp;
@end

@implementation AClass
- (void)anInstanceMethod {
    NSLog(@"instance method");
}
@end

int main(void)
{
  AClass *instance = [AClass init];
  [instance anInstanceMethod];
  instance.aProp = 12;
}

/*
OUTPUT:
{
  "includes": [{
      "line": 1,
      "resolved_path": "&Foundation.h"
    }],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:objc(cs)AClass",
      "short_name": "AClass",
      "detailed_name": "AClass",
      "definition_spelling": "9:17-9:23",
      "definition_extent": "9:1-13:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["3:12-3:18", "9:17-9:23", "17:3-17:9", "17:23-17:29"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:objc(cs)AClass(im)anInstanceMethod",
      "short_name": "anInstanceMethod",
      "detailed_name": " AClass::anInstanceMethod",
      "declarations": [{
          "spelling": "4:11-4:27",
          "extent": "4:3-4:28",
          "content": "- (void)anInstanceMethod;",
          "param_spellings": []
        }],
      "definition_spelling": "10:9-10:25",
      "definition_extent": "10:1-12:2",
      "derived": [],
      "locals": [],
      "callers": ["4@18:13-18:29"],
      "callees": []
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:objc(cs)AClass(im)aProp",
      "short_name": "aProp",
      "detailed_name": " AClass::aProp",
      "declarations": [{
          "spelling": "0:0-0:0",
          "extent": "5:29-5:34",
          "content": "aProp",
          "param_spellings": []
        }],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }, {
      "id": 2,
      "is_operator": false,
      "usr": "c:objc(cs)AClass(im)setAProp:",
      "short_name": "setAProp:",
      "detailed_name": " AClass::setAProp:",
      "declarations": [{
          "spelling": "0:0-0:0",
          "extent": "5:29-5:34",
          "content": "aProp",
          "param_spellings": ["5:29-5:34"]
        }],
      "derived": [],
      "locals": [],
      "callers": ["4@0:0-0:0"],
      "callees": []
    }, {
      "id": 3,
      "is_operator": false,
      "usr": "c:objc(cs)AClass(im)readOnlyProp",
      "short_name": "readOnlyProp",
      "detailed_name": " AClass::readOnlyProp",
      "declarations": [{
          "spelling": "0:0-0:0",
          "extent": "6:28-6:40",
          "content": "readOnlyProp",
          "param_spellings": []
        }],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }, {
      "id": 4,
      "is_operator": false,
      "usr": "c:@F@main#",
      "short_name": "main",
      "detailed_name": "int main()",
      "declarations": [],
      "definition_spelling": "15:5-15:9",
      "definition_extent": "15:1-20:2",
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": ["0@18:13-18:29", "2@0:0-0:0"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:objc(cs)AClass(py)aProp",
      "short_name": "aProp",
      "detailed_name": "int AClass::aProp",
      "declaration": "5:29-5:34",
      "is_local": true,
      "is_macro": false,
      "uses": ["5:29-5:34", "19:12-19:17"]
    }, {
      "id": 1,
      "usr": "c:objc(cs)AClass(py)readOnlyProp",
      "short_name": "readOnlyProp",
      "detailed_name": "int AClass::readOnlyProp",
      "declaration": "6:28-6:40",
      "is_local": true,
      "is_macro": false,
      "uses": ["6:28-6:40"]
    }, {
      "id": 2,
      "usr": "c:objc(cs)AClass@_aProp",
      "short_name": "_aProp",
      "detailed_name": "int AClass::_aProp",
      "definition_spelling": "5:29-5:34",
      "definition_extent": "5:29-5:34",
      "is_local": true,
      "is_macro": false,
      "uses": ["5:29-5:34"]
    }, {
      "id": 3,
      "usr": "c:objc(cs)AClass@_readOnlyProp",
      "short_name": "_readOnlyProp",
      "detailed_name": "int AClass::_readOnlyProp",
      "definition_spelling": "6:28-6:40",
      "definition_extent": "6:28-6:40",
      "is_local": true,
      "is_macro": false,
      "uses": ["6:28-6:40"]
    }, {
      "id": 4,
      "usr": "c:class.m@297@F@main#@instance",
      "short_name": "instance",
      "detailed_name": "AClass * instance",
      "definition_spelling": "17:11-17:19",
      "definition_extent": "17:3-17:35",
      "is_local": true,
      "is_macro": false,
      "uses": ["17:11-17:19", "18:4-18:12", "19:3-19:11"]
    }]
}
*/
