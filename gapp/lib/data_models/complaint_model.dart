class ComplaintModel {
  String complaintId = "";
  String email = "";
  String title = "";
  String complaint = "";
  String userId = "";
  int createdOn = 0;

  ComplaintModel({
    required this.complaint,
    required this.userId,
    required this.createdOn,
    required this.title,
    required this.email,
    required this.complaintId,
  });

  ComplaintModel.fromJson(Map<String, dynamic> json) {
    complaintId = json['complaintId'];
    title = json['title'];
    complaint = json['complaint'];
    email = json['email'];
    userId = json['userId'];
    createdOn = json['createdOn'];
  }

  Map<String, dynamic> toJson() {
    final data = <String, dynamic>{
      'email': email,
      'title': title,
      'complaint': complaint,
      'userId': userId,
      'createdOn': createdOn,
      'complaintId':complaintId
    };
    return data;
  }
}
