import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/widgets/sub_txt_widget.dart';

import '../utils/colors.dart';
import 'header_txt_widget.dart';

class TimeWidget extends StatefulWidget {
  final ValueChanged<String> onChanged;
  String? selectedTime;

  TimeWidget({Key? key, required this.onChanged, this.selectedTime}) : super(key: key);

  @override
  _CustomSwitchState createState() => _CustomSwitchState();
}

class _CustomSwitchState extends State<TimeWidget> {
  String? selectedTime;
  String? selectedTimeType;
  List<String> timeList = [
    '1:00',
    "1:30",
    '2:00',
    '2:30',
    '3:00',
    '3:30',
    '4:00',
    '4:30',
    '5:00',
    '5:30',
    '6:00',
    '6:30',
    '7:00',
    '7:30',
    '8:00',
    '8:30',
    '9:00',
    '9:30',
    '10:00',
    '10:30',
    '11:00',
    '11:30',
    '12:00',
    '12:30'
  ];

  @override
  void initState() {
    super.initState();
    if(widget.selectedTime!=null) {
      List time=widget.selectedTime!.split(" ");
      if(time.length==2) {
        selectedTime = time[0];
        selectedTimeType = time[1];
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Material(
      color: Colors.transparent,
      child: Stack(
        children: [
          Center(
            child: Wrap(
              children: [
                Container(
                  margin: const EdgeInsets.all(20),
                  padding: const EdgeInsets.all(20),
                  decoration: const BoxDecoration(
                      borderRadius: BorderRadius.all(Radius.circular(10)),
                      color: Colors.white),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      HeaderTxtWidget('Select Time'),
                      const SizedBox(height: 10,),
                      Center(
                        child: Wrap(
                          children: [
                            InkWell(
                              onTap: () {
                                setState(() {
                                  selectedTimeType="AM";
                                });
                              },
                              child: Container(
                                padding: const EdgeInsets.all(10),
                                margin: const EdgeInsets.all(5),
                                decoration: BoxDecoration(
                                  borderRadius: const BorderRadius.all(Radius.circular(10)),
                                  border: Border.all(color: Colors.grey, width: 1),
                                    color: selectedTimeType=="AM"?primaryColorCode:Colors.white
                                ),
                                child: SubTxtWidget("AM",color: selectedTimeType=="AM"?Colors.white:Colors.black,),
                              ),
                            ),
                            InkWell(
                              onTap: () {
                                setState(() {
                                  selectedTimeType="PM";
                                });
                              },
                              child: Container(
                                padding: const EdgeInsets.all(10),
                                margin: const EdgeInsets.all(5),
                                decoration: BoxDecoration(
                                  borderRadius: const BorderRadius.all(Radius.circular(10)),
                                  border: Border.all(color: Colors.grey, width: 1),
                                    color: selectedTimeType=="PM"?primaryColorCode:Colors.white
                                ),
                                child: SubTxtWidget("PM",color: selectedTimeType=="PM"?Colors.white:Colors.black,),
                              ),
                            ),
                          ],
                        ),
                      ),
                      const SizedBox(height: 10,),
                      GridView.builder(
                        gridDelegate:  const SliverGridDelegateWithFixedCrossAxisCount(
                            crossAxisCount: 4,
                            childAspectRatio: 1.5),
                        itemBuilder: (context, index) {
                          return _tile(timeList[index],onTap: (time) {
                            setState(() {
                              selectedTime=time;
                            });
                          },);
                        },
                        itemCount: timeList.length,
                        shrinkWrap: true,
                      ),
                      const SizedBox(height: 20,),
                      if(widget.selectedTime!=null)
                      Center(
                        child: HeaderTxtWidget('Selected time:$selectedTime $selectedTimeType'),
                      ),
                      if(selectedTime==null||selectedTimeType==null)
                      Center(
                        child: SubTxtWidget('Please select time & AM/PM',color: Colors.red,),
                      ),
                      const SizedBox(height: 20,),
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                        children: [
                          ActionChip(label: SubTxtWidget('Submit',color: Colors.white,),onPressed: () {
                            if(selectedTime!=null&&selectedTimeType!=null) {
                              widget.onChanged(
                                  "$selectedTime $selectedTimeType");
                              Get.back();
                            }
                          },color: WidgetStatePropertyAll(primaryColorCode),),
                          ActionChip(label: SubTxtWidget('Cancel',),onPressed: () {
                            Get.back();
                          })

                        ],
                      )
                    ],
                  ),
                )
              ],
            ),
          )
        ],
      ),
    );
  }

  Widget _tile(String txt,{required Function(String) onTap}) {
    return InkWell(
      onTap: () {
        onTap.call(txt);
      },
      child: Container(
        padding: const EdgeInsets.all(10),
        margin: const EdgeInsets.all(5),
        alignment: AlignmentDirectional.center,
        decoration: BoxDecoration(
          borderRadius: const BorderRadius.all(Radius.circular(10)),
          border: Border.all(color: Colors.grey, width: 1),
          color: selectedTime==txt?primaryColorCode:Colors.white
        ),
        child: SubTxtWidget(txt,color: selectedTime==txt?Colors.white:Colors.black,),
      ),
    );
  }
}
