class ReportModel {
  ReportModel({
      this.providerId, 
      this.createdDate, 
      this.id, 
      this.region, 
      this.clientId, 
      this.status,});

  ReportModel.fromJson(dynamic json) {
    providerId = json['provider_id'];
    createdDate = json['created_date'];
    id = json['id'];
    region = json['region'];
    clientId = json['client_id'];
    status = json['status'];
  }
  String? providerId;
  String? createdDate;
  String? id;
  String? region;
  String? clientId;
  String? status;

  Map<String, dynamic> toJson() {
    final map = <String, dynamic>{};
    map['provider_id'] = providerId;
    map['created_date'] = createdDate;
    map['id'] = id;
    map['region'] = region;
    map['client_id'] = clientId;
    map['status'] = status;
    return map;
  }

}