class NotificationModel {
  String notificationText = '';
  String userId = '';
  int createdOn = 0;
  String notificationId = "";
  String type="";
  String serviceId="";
  String sendBy="";

  NotificationModel(
      {required this.userId,
      required this.createdOn,
      required this.notificationText,
      required this.type,
      required this.serviceId,
      required this.sendBy,
      required this.notificationId});

  NotificationModel.fromJson(Map<String, dynamic> json) {
    notificationText = json['notificationText'];
    userId = json['userId'];
    createdOn = json['createdOn'];
    sendBy = json['sendBy'];
    serviceId = json['serviceId'];
    type = json['type'];
    notificationId = json['notificationId'];
  }

  Map<String, dynamic> toJson() {
    final data = <String, dynamic>{
      "notificationId": notificationId,
      "notificationText": notificationText,
      "userId": userId,
      "createdOn": createdOn,
      "serviceId": serviceId,
      "type": type,
      "sendBy": sendBy
    };
    return data;
  }
}
