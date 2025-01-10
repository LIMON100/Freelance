class RatingModel {
  double ratingValue;
  String ratingId;
  String providerId;
  String userId;
  String? comment;
  int createdOn;

  RatingModel({
    required this.ratingValue,
    required this.userId,
    required this.providerId,
    required this.ratingId,
    this.comment,
    required this.createdOn,
  });

  RatingModel.fromJson(Map<String, dynamic> json)
      : providerId = json['providerId'],
        comment = json['comment'],
        ratingId = json['ratingId'],
        ratingValue = json['ratingValue'].toDouble(),
        userId = json['userId'],
        createdOn = json['createdOn'];

  Map<String, dynamic> toJson() {
    final data = <String, dynamic>{};
    data['ratingValue'] = ratingValue;
    data['userId'] = userId;
    data['createdOn'] = createdOn;
    data['ratingId'] = ratingId;
    data['providerId'] = providerId;
    data['comment'] = comment;
    return data;
  }
}
