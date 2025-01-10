class TransactionModel {
  TransactionModel({
      this.paymentFor, 
      this.amount, 
      this.createdDate, 
      this.paymentMode, 
      this.providerId, 
      this.id, 
      this.userId, 
      this.transactionId,});

  TransactionModel.fromJson(dynamic json) {
    paymentFor = json['paymentFor'];
    amount = json['amount'];
    createdDate = json['createdDate'];
    paymentMode = json['paymentMode'];
    providerId = json['providerId'];
    id = json['id'];
    userId = json['userId'];
    transactionId = json['transactionId'];
  }
  dynamic paymentFor;
  int? amount;
  String? createdDate;
  String? paymentMode;
  String? providerId;
  String? id;
  String? userId;
  String? transactionId;

  Map<String, dynamic> toJson() {
    final map = <String, dynamic>{};
    map['paymentFor'] = paymentFor;
    map['amount'] = amount;
    map['createdDate'] = createdDate;
    map['paymentMode'] = paymentMode;
    map['providerId'] = providerId;
    map['id'] = id;
    map['userId'] = userId;
    map['transactionId'] = transactionId;
    return map;
  }

}