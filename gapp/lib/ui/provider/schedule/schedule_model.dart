class ScheduleModel {
  ScheduleModel({
      this.dayName, 
      this.daySortName,
      this.fromTime,
      this.toTime, 
      this.isEnable,
      this.id,});

  ScheduleModel.fromJson(dynamic json) {
    dayName = json['dayName'];
    daySortName = json['daySortName'];
    fromTime = json['fromTime'];
    toTime = json['toTime'];
    isEnable = json['isEnable']??false;
    id = json['id'];
  }
  String? dayName;
  String? daySortName;
  String? fromTime;
  String? toTime;
  bool? isEnable;
  String? id;

  Map<String, dynamic> toJson() {
    final map = <String, dynamic>{};
    map['dayName'] = dayName;
    map['daySortName'] = daySortName;
    map['fromTime'] = fromTime;
    map['toTime'] = toTime;
    map['isEnable'] = isEnable;
    map['id'] = id;
    return map;
  }

}