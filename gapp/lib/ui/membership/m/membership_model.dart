class MembershipModel{
  String title;
  String duration;
  String? startDate;
  String? endDate;
  String amount;
  double totalAmount;
  bool isSelected;
  MembershipModel({required this.title,required this.duration,required this.amount,
    required this.totalAmount,this.isSelected=false,
    this.startDate,this.endDate
  });
  factory MembershipModel.fromJson(Map<String, dynamic> json) {
    return MembershipModel(
      title: json['title'],
      totalAmount: json['totalAmount'],
      duration: json['duration'],
      amount: json['amount'],
      startDate: json['startDate'],
      endDate: json['endDate'],
    );
  }
  Map<String, dynamic> toJson() {
    final data = <String, dynamic>{
      'startDate': startDate,
      'endDate': endDate,
      'title': title,
      'duration': duration,
      'amount': amount,
      'totalAmount': totalAmount,
    };
    return data;
  }
}