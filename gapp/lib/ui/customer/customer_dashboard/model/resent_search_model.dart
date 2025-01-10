class ResentSearchModel {
  ResentSearchModel({
      this.query, 
      this.id,});

  ResentSearchModel.fromJson(dynamic json) {
    query = json['query'];
    id = json['id'];
  }
  String? query;
  String? id;

  Map<String, dynamic> toJson() {
    final map = <String, dynamic>{};
    map['query'] = query;
    map['id'] = id;
    return map;
  }

}