@interface AClass
  + (void)test;
  - (void)anInstanceMethod;
  @property (nonatomic) int aProp;
@end

@implementation AClass
+ (void)test {}
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
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 4096877434426330804,
      "detailed_name": "- (void)AClass::anInstanceMethod;",
      "qual_name_offset": 2,
      "short_name": "anInstanceMethod",
      "spell": "9:9-9:25|9:1-9:28|1090|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 11,
      "storage": 0,
      "declarations": ["3:11-3:27|3:3-3:28|1089|-1"],
      "derived": [],
      "uses": ["15:13-15:29|24676|-1"]
    }, {
      "usr": 7924728095432766067,
      "detailed_name": "int main(void)",
      "qual_name_offset": 4,
      "short_name": "main",
      "spell": "12:5-12:9|12:1-17:2|2|-1",
      "bases": [],
      "vars": [11068172662702654556],
      "callees": ["15:13-15:29|4096877434426330804|3|24676", "16:12-16:17|17992064398538597892|3|24932"],
      "kind": 12,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 12774569141855220778,
      "detailed_name": "- (int)AClass::aProp;",
      "qual_name_offset": 2,
      "short_name": "aProp",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["4:29-4:34|4:29-4:34|1345|-1"],
      "derived": [],
      "uses": []
    }, {
      "usr": 12775970426728664910,
      "detailed_name": "+ (void)AClass::test;",
      "qual_name_offset": 2,
      "short_name": "test",
      "spell": "8:9-8:13|8:1-8:16|1090|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 11,
      "storage": 0,
      "declarations": ["2:11-2:15|2:3-2:16|1089|-1"],
      "derived": [],
      "uses": []
    }, {
      "usr": 17992064398538597892,
      "detailed_name": "- (void)AClass::setAProp:(int)aProp;",
      "qual_name_offset": 2,
      "short_name": "setAProp:",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["4:29-4:34|4:29-4:34|1345|-1"],
      "derived": [],
      "uses": ["16:12-16:17|24932|-1"]
    }],
  "usr2type": [{
      "usr": 11832280568361305387,
      "detailed_name": "@implementation AClass\n@end",
      "qual_name_offset": 16,
      "short_name": "AClass",
      "spell": "7:17-7:23|7:1-10:2|2|-1",
      "bases": [],
      "funcs": [12775970426728664910, 4096877434426330804, 12774569141855220778, 17992064398538597892],
      "types": [14842397373703114213],
      "vars": [],
      "alias_of": 0,
      "kind": 11,
      "parent_kind": 1,
      "declarations": ["1:12-1:18|1:1-5:5|1|-1"],
      "derived": [],
      "instances": [],
      "uses": ["14:3-14:9|4|-1", "14:23-14:29|4|-1"]
    }, {
      "usr": 14842397373703114213,
      "detailed_name": "@property(nonatomic, assign, unsafe_unretained, readwrite) int AClass::aProp;",
      "qual_name_offset": 63,
      "short_name": "aProp",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 7,
      "parent_kind": 0,
      "declarations": ["4:29-4:34|4:3-4:34|1025|-1"],
      "derived": [],
      "instances": [],
      "uses": ["16:12-16:17|20|-1"]
    }],
  "usr2var": [{
      "usr": 11068172662702654556,
      "detailed_name": "AClass *instance",
      "qual_name_offset": 8,
      "short_name": "instance",
      "hover": "AClass *instance = [AClass init]",
      "spell": "14:11-14:19|14:3-14:35|2|-1",
      "type": 0,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": ["15:4-15:12|12|-1", "16:3-16:11|12|-1"]
    }]
}
*/
