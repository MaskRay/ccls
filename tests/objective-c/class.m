@interface AClass
  + (void)test;
  - (void)anInstanceMethod;
  @property (nonatomic) int aProp;
@end

@implementation AClass
- (void)anInstanceMethod {}
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
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:objc(cs)AClass",
      "short_name": "AClass",
      "detailed_name": "AClass",
      "definition_spelling": "7:17-7:23",
      "definition_extent": "7:1-9:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["1:12-1:18", "7:17-7:23", "13:3-13:9", "13:23-13:29"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:objc(cs)AClass(cm)test",
      "short_name": "test",
      "detailed_name": " AClass::test",
      "declarations": [{
          "spelling": "2:11-2:15",
          "extent": "2:3-2:16",
          "content": "+ (void)test;",
          "param_spellings": []
        }],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:objc(cs)AClass(im)anInstanceMethod",
      "short_name": "anInstanceMethod",
      "detailed_name": " AClass::anInstanceMethod",
      "declarations": [{
          "spelling": "3:11-3:27",
          "extent": "3:3-3:28",
          "content": "- (void)anInstanceMethod;",
          "param_spellings": []
        }],
      "definition_spelling": "8:9-8:25",
      "definition_extent": "8:1-8:28",
      "derived": [],
      "locals": [],
      "callers": ["4@14:13-14:29"],
      "callees": []
    }, {
      "id": 2,
      "is_operator": false,
      "usr": "c:objc(cs)AClass(im)aProp",
      "short_name": "aProp",
      "detailed_name": " AClass::aProp",
      "declarations": [{
          "spelling": "0:0-0:0",
          "extent": "4:29-4:34",
          "content": "aProp",
          "param_spellings": []
        }],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }, {
      "id": 3,
      "is_operator": false,
      "usr": "c:objc(cs)AClass(im)setAProp:",
      "short_name": "setAProp:",
      "detailed_name": " AClass::setAProp:",
      "declarations": [{
          "spelling": "0:0-0:0",
          "extent": "4:29-4:34",
          "content": "aProp",
          "param_spellings": ["4:29-4:34"]
        }],
      "derived": [],
      "locals": [],
      "callers": ["4@0:0-0:0"],
      "callees": []
    }, {
      "id": 4,
      "is_operator": false,
      "usr": "c:@F@main#",
      "short_name": "main",
      "detailed_name": "int main()",
      "declarations": [],
      "definition_spelling": "11:5-11:9",
      "definition_extent": "11:1-16:2",
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": ["1@14:13-14:29", "3@0:0-0:0"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:objc(cs)AClass(py)aProp",
      "short_name": "aProp",
      "detailed_name": "int AClass::aProp",
      "declaration": "4:29-4:34",
      "is_local": true,
      "is_macro": false,
      "uses": ["4:29-4:34", "15:12-15:17"]
    }, {
      "id": 1,
      "usr": "c:class.m@191@F@main#@instance",
      "short_name": "instance",
      "detailed_name": "AClass * instance",
      "definition_spelling": "13:11-13:19",
      "definition_extent": "13:3-13:35",
      "is_local": true,
      "is_macro": false,
      "uses": ["13:11-13:19", "14:4-14:12", "15:3-15:11"]
    }]
}
*/
