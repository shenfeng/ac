struct AcRespItem {
    1: required string item,
    3: required double score,
    4: optional string highlighted;
}

struct AcResp  {
    1: required i32 total,
    2: required list<AcRespItem> items,
}

struct AcReq {
    1: required string kind,
    2: required string q,
    3: required i32 limit = 10,

    // track user
    4: optional string uuid,
    5: optional string uid,
    6: optional i32 debug,

    7: optional i32 offset = 0,
    8: optional bool highlight = true,
}

service AutoComplete {
       AcResp autocomplete(1: AcReq req)
       list<string> acnames()
}
