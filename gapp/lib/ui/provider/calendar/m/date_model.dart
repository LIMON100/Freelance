class DateModel{
  bool selected;
  String date;
  String fullDate;
  String day;
  DateTime? startDate;
  DateTime? endDate;
  DateModel({required this.selected,required this.date, required this.day,required this.fullDate,
   this.startDate, this.endDate});
}