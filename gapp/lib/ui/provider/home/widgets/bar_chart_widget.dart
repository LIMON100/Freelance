import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/header_txt_widget.dart';

import '../../../../utils/tools.dart';
import '../../../../firebase/provider_user_firebase.dart';
import '../../../../repo/setting_repo.dart';
import '../../../../widgets/sub_txt_widget.dart';
import '../m/transaction.dart';

class BarChartWidget extends StatefulWidget {
  BarChartWidget({super.key});
  final Color leftBarColor = primaryColorCode;
  final Color rightBarColor = primaryColorCode;
  final Color avgColor =Colors.blue;
  @override
  State<StatefulWidget> createState() => BarChartState();
}

class BarChartState extends State<BarChartWidget> {
  final double width = 7;
   List<BarChartGroupData> showingBarGroups=[];
   List<BarChartGroupData> rawBarGroups=[];
  int touchedGroupIndex = -1;
  String filter="Week";

  @override
  void initState() {
    super.initState();
    initValues();
  }
  Future<void> initValues() async {
    showingBarGroups.clear();
    List<TransactionModel> list=await ProviderUserFirebase().getAllTransaction(auth.value.uid);
    bindWeekData(list);
    bindMonthData(list);
    bindYearData(list);
  }
  void bindWeekData(List<TransactionModel> list){
    List<double>total=[0,0,0,0,0,0,0];
    DateTime now=DateTime.now();
    for (var element in list) {
      DateTime day=Tools.changeToDate(element.createdDate!);
      if(day.isAfter(DateTime(now.year,now.month,now.day-now.weekday))){
        total[day.weekday-1]=total[day.weekday]+element.amount!;
      }
    }
    if(filter=="Week"){
      for(int i=0;i<total.length;i++){
        showingBarGroups.add(makeGroupData(i, 0, total[i]/1000));
      }
    }
    setState(() {
      rawBarGroups=showingBarGroups;
    });
  }
  void bindMonthData(List<TransactionModel> list){
    List<double>total=[0,0,0,0,0,0,0,0,0,0,0,0];

    if(filter=="Month"){
      for(int i=0;i<total.length;i++){
        for (var element in list) {
          DateTime day=Tools.changeToDate(element.createdDate!);
          if(day.month==(i+1)) {
            total[i]=total[i]+element.amount!;
          }
        }
        showingBarGroups.add(makeGroupData(i, 0, total[i]/1000));
      }
    }
    setState(() {
      rawBarGroups=showingBarGroups;
    });
  }
  void bindYearData(List<TransactionModel> list){
    List<double>total=[0,0,0];
    List<double>year=[2023,2024,2025];
    if(filter=="Year"){
      for(int i=0;i<total.length;i++){
        for (var element in list) {
          DateTime day=Tools.changeToDate(element.createdDate!);
          if(day.year==year[i]) {
            total[i]=total[i]+element.amount!;
          }
        }
        showingBarGroups.add(makeGroupData(i, 0, total[i]/1000));
      }
    }
    setState(() {
      rawBarGroups=showingBarGroups;
    });
  }

  @override
  Widget build(BuildContext context) {
    return AspectRatio(
      aspectRatio: 1,
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: <Widget>[
            Row(
              mainAxisSize: MainAxisSize.min,
              children: <Widget>[
                Container(
                  width: 100,
                  height: 40,
                  margin: const EdgeInsets.only(right: 10),
                  child: DropdownButtonFormField(
                    items: ["Week", "Month", "Year"].map(
                          (e) {
                        return DropdownMenuItem(
                          value: e,
                          child: SubTxtWidget(
                            e,
                            color: Colors.white,
                          ),
                        );
                      },
                    ).toList(),
                    dropdownColor: Colors.blue,
                    value: filter,
                    onChanged: (value) {
                      setState(() {
                        filter=value!;
                        initValues();
                      });
                    },
                    decoration: InputDecoration(
                      border: const OutlineInputBorder(
                          borderRadius: BorderRadius.all(
                              Radius.circular(20)),
                          borderSide: BorderSide.none),
                      fillColor: "#399DDC".toColor(),
                      filled: true,
                      contentPadding: const EdgeInsets.symmetric(
                          horizontal: 10),
                    ),
                    iconEnabledColor: Colors.white,
                  ),
                ),
                const SizedBox(
                  width: 10,
                ),
                Expanded(
                    child: Text(
                      "Monthly Revenue USD",
                      style: GoogleFonts.workSans(
                          fontSize: 18,
                          fontWeight: FontWeight.bold,
                          color: titleColor),
                    ))
              ],
            ),
            const SizedBox(
              height: 38,
            ),
            Expanded(
              child: BarChart(
                BarChartData(
                  maxY: 20,
                  barTouchData: BarTouchData(
                    touchTooltipData: BarTouchTooltipData(
                      getTooltipColor: ((group) {
                        return Colors.grey;
                      }),
                      getTooltipItem: (a, b, c, d) => null,
                    ),
                    touchCallback: (FlTouchEvent event, response) {
                      if (response == null || response.spot == null) {
                        setState(() {
                          touchedGroupIndex = -1;
                          showingBarGroups = List.of(rawBarGroups);
                        });
                        return;
                      }

                      touchedGroupIndex = response.spot!.touchedBarGroupIndex;

                      setState(() {
                        if (!event.isInterestedForInteractions) {
                          touchedGroupIndex = -1;
                          showingBarGroups = List.of(rawBarGroups);
                          return;
                        }
                        showingBarGroups = List.of(rawBarGroups);
                        if (touchedGroupIndex != -1) {
                          var sum = 0.0;
                          for (final rod
                          in showingBarGroups[touchedGroupIndex].barRods) {
                            sum += rod.toY;
                          }
                          final avg = sum /
                              showingBarGroups[touchedGroupIndex]
                                  .barRods
                                  .length;

                          showingBarGroups[touchedGroupIndex] =
                              showingBarGroups[touchedGroupIndex].copyWith(
                                barRods: showingBarGroups[touchedGroupIndex]
                                    .barRods
                                    .map((rod) {
                                  return rod.copyWith(
                                      toY: avg, color: widget.avgColor);
                                }).toList(),
                              );
                        }
                      });
                    },
                  ),
                  titlesData: FlTitlesData(
                    show: true,
                    rightTitles: const AxisTitles(
                      sideTitles: SideTitles(showTitles: false),
                    ),
                    topTitles: const AxisTitles(
                      sideTitles: SideTitles(showTitles: false),
                    ),
                    bottomTitles: AxisTitles(
                      sideTitles: SideTitles(
                        showTitles: true,
                        getTitlesWidget: bottomTitles,
                        reservedSize: 42,
                      ),
                    ),
                    leftTitles: AxisTitles(
                      sideTitles: SideTitles(
                        showTitles: true,
                        reservedSize: 28,
                        getTitlesWidget: leftTitles,
                      ),
                    ),
                  ),
                  borderData: FlBorderData(
                    show: false,
                  ),
                  barGroups: showingBarGroups,
                  gridData: const FlGridData(show: false),
                ),
              ),
            ),
            const SizedBox(
              height: 12,
            ),
          ],
        ),
      ),
    );
  }

  Widget leftTitles(double value, TitleMeta meta) {
    const style = TextStyle(
      color: Color(0xff7589a2),
      fontWeight: FontWeight.bold,
      fontSize: 14,
    );
    String text;
    if (value == 0) {
      text = '1K';
    } else if (value == 10) {
      text = '5K';
    } else if (value == 19) {
      text = '10K';
    } else {
      return Container();
    }
    return SideTitleWidget(
      axisSide: meta.axisSide,
      space: 0,
      child: Text(text, style: style),
    );
  }

  Widget bottomTitles(double value, TitleMeta meta) {
    var titles=[];
    if(filter=="Week") {
      final weekList = <String>['Mn', 'Tu', 'Wd', 'Th', 'Fr', 'St', 'Su'];
      titles=weekList;
    }
    if(filter=="Month") {
      final monthList = <String>['Jan', 'Fab', 'Mar', 'Apr', 'May', 'Jun', 'Jul','Aug','Sep','Oct','Nov','Dec'];
      titles=monthList;
    }
    if(filter=="Year") {
      final yearList = <String>['2023', '2024', '2025'];
      titles=yearList;
    }
    final Widget text = Text(
      titles[value.toInt()],
      style:  TextStyle(
        color: const Color(0xff7589a2),
        fontWeight: FontWeight.bold,
        fontSize: filter=="Month"?10:14,
      ),
    );

    return SideTitleWidget(
      axisSide: meta.axisSide,
      space: 16, //margin top
      child: text,
    );
  }


  BarChartGroupData makeGroupData(int x, double y1, double y2) {
    return BarChartGroupData(
      barsSpace: 4,
      x: x,
      barRods: [
        BarChartRodData(
          toY: y1,
          color: widget.leftBarColor,
          width: width,
        ),
        BarChartRodData(
          toY: y2,
          color: widget.rightBarColor,
          width: width,
        ),
      ],
    );
  }

 }