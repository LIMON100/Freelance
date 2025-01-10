import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/ui/provider/schedule/schedule_model.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/custom_switch.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import 'package:time_range_picker/time_range_picker.dart';

import 'schedule_controller.dart';

class SchedulePage extends StatefulWidget {
  const SchedulePage({super.key});

  @override
  State<SchedulePage> createState() => _ScreenState();
}

class _ScreenState extends State<SchedulePage> {
  final con = Get.put(ScheduleController());
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.white,
        title: HeaderTxtWidget('Default Work Schedule'),
        iconTheme: IconThemeData(color: primaryColorCode),
        actions: [
          TextButton(
              onPressed: () {
                con.saveSchedule();
              },
              child: SubTxtWidget(
                'Done',
                color: Colors.blue,
              ))
        ],
      ),
      body: SingleChildScrollView(
        child: Container(
          padding: const EdgeInsets.all(10),
          child: Obx(
            () => Column(
              children: [
                Row(
                  children: con.scheduleList.map(
                    (element) {
                      return Expanded(
                        flex: 1,
                        child: InkWell(
                          onTap: () {
                            con.scheduleList
                                .firstWhere(
                                  (de) => de.daySortName == element.daySortName,
                                )
                                .isEnable = true;
                            setState(() {});
                          },
                          child: Container(
                            padding: const EdgeInsets.all(5),
                            margin: const EdgeInsets.symmetric(horizontal: 5),
                            alignment: AlignmentDirectional.center,
                            decoration: BoxDecoration(
                                borderRadius:
                                    const BorderRadius.all(Radius.circular(10)),
                                color: Colors.white,
                                border: Border.all(
                                    color: element.isEnable!
                                        ? primaryColorCode
                                        : Colors.white,
                                    width: 1)),
                            child: SubTxtWidget(
                              element.daySortName!,
                              fontSize: 12,
                            ),
                          ),
                        ),
                      );
                    },
                  ).toList(),
                ),
                Container(
                  margin:
                      const EdgeInsets.symmetric(vertical: 20, horizontal: 5),
                  padding:
                      const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
                  decoration: BoxDecoration(
                      color: "#C2B5FF".toColor().withOpacity(0.42),
                      borderRadius:
                          const BorderRadius.all(Radius.circular(10))),
                  child: Row(
                    children: [
                      Expanded(
                          child: SubTxtWidget('Use same hours for all days')),
                      CustomSwitch(
                        value: con.useSameTime.value,
                        onChanged: (value) {
                          con.useSameTime.value = value;
                        },
                        borderColor: Colors.red,
                        innerColor: Color(0x363062).withOpacity(1),
                      )
                    ],
                  ),
                ),
                ListView.builder(
                  itemBuilder: (context, index) {
                    ScheduleModel data = con.scheduleList
                        .where(
                          (e) => e.isEnable!,
                        )
                        .toList()[index];
                    return Container(
                      padding: const EdgeInsets.symmetric(
                          horizontal: 10, vertical: 5),
                      child: Row(
                        children: [
                          Expanded(
                            flex: 3,
                            child: SubTxtWidget(data.dayName!),
                          ),
                          Expanded(
                            flex: 5,
                            child: InkWell(
                              onTap: () async {
                                TimeRange result = await showTimeRangePicker(
                                    context: context, rotateLabels: true);
                                data.fromTime =
                                    result.startTime.format(context);
                                data.toTime = result.endTime.format(context);
                                con.updateAllTime(
                                    result.startTime.format(context),
                                    result.endTime.format(context));
                                setState(() {});
                              },
                              child: Container(
                                alignment: AlignmentDirectional.center,
                                padding: const EdgeInsets.all(5),
                                decoration: BoxDecoration(
                                    border: Border.all(
                                        color: '#67D0E6'.toColor(), width: 0.5),
                                    borderRadius: const BorderRadius.all(
                                        Radius.circular(10))),
                                child: SubTxtWidget(
                                    '${data.fromTime!}-${data.toTime!}'),
                              ),
                            ),
                          ),
                          IconButton(
                              onPressed: () {
                                setState(() {
                                  data.isEnable = false;
                                });
                              },
                              icon: const Icon(
                                Icons.delete_outline,
                                color: Colors.red,
                              )),
                        ],
                      ),
                    );
                  },
                  itemCount: con.scheduleList
                      .where(
                        (e) => e.isEnable!,
                      )
                      .toList()
                      .length,
                  primary: false,
                  shrinkWrap: true,
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
