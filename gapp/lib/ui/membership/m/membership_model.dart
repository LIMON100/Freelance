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

// class MembershipModel {
//   String title;
//   String duration;
//   String? startDate;
//   String? endDate;
//   String amount;
//   double totalAmount;
//   bool isSelected;
//   bool isDowngradeAllowed; // New property for downgrade eligibility
//
//   MembershipModel({
//     required this.title,
//     required this.duration,
//     required this.amount,
//     required this.totalAmount,
//     this.isSelected = false,
//     this.startDate,
//     this.endDate,
//     this.isDowngradeAllowed = false, // Default value
//   });
//
//   factory MembershipModel.fromJson(Map<String, dynamic> json) {
//     return MembershipModel(
//       title: json['title'],
//       totalAmount: json['totalAmount'],
//       duration: json['duration'],
//       amount: json['amount'],
//       startDate: json['startDate'],
//       endDate: json['endDate'],
//       isDowngradeAllowed: json['isDowngradeAllowed'] ?? false, // Ensure it defaults to false if not present
//     );
//   }
//
//   Map<String, dynamic> toJson() {
//     final data = <String, dynamic>{
//       'startDate': startDate,
//       'endDate': endDate,
//       'title': title,
//       'duration': duration,
//       'amount': amount,
//       'totalAmount': totalAmount,
//       'isDowngradeAllowed': isDowngradeAllowed, // Include downgrade eligibility in JSON
//     };
//     return data;
//   }
// }