class ReviewModel {
  ReviewModel({
      this.providerId, 
      this.serviceId,
      this.ratingId,
      this.ratingValue, 
      this.comment, 
      this.createdOn, 
      this.updated, 
      this.userId, 
      this.status,});

  ReviewModel.fromJson(dynamic json) {
    providerId = json['providerId'];
    serviceId = json['serviceId'];
    ratingId = json['ratingId'];
    ratingValue = json['ratingValue']!=null?double.tryParse(json['ratingValue'].toString()):0;
    comment = json['comment'];
    createdOn = json['createdOn'];
    updated = json['updated'];
    userId = json['userId'];
    status = json['status'];
  }
  String? providerId;
  String? ratingId;
  String? serviceId;
  double? ratingValue;
  String? comment;
  int? createdOn;
  bool? updated;
  String? userId;
  bool? status;

  Map<String, dynamic> toJson() {
    final map = <String, dynamic>{};
    map['providerId'] = providerId;
    map['ratingId'] = ratingId;
    map['ratingValue'] = ratingValue;
    map['comment'] = comment;
    map['createdOn'] = createdOn;
    map['updated'] = updated;
    map['userId'] = userId;
    map['status'] = status;
    map['serviceId'] = serviceId;
    return map;
  }

}