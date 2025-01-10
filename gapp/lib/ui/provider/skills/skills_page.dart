import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/loading_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../../../utils/colors.dart';
import '../../../widgets/header_txt_widget.dart';
import 'skills_controller.dart';

class SkillsPage extends StatefulWidget {
  const SkillsPage({super.key});

  @override
  State<SkillsPage> createState() => _ScreenState();
}

class _ScreenState extends State<SkillsPage> {
  final con = Get.put(SkillsController());

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.white,
        title: HeaderTxtWidget('Skills'),
        iconTheme: IconThemeData(color: primaryColorCode),
      ),
      body: Container(
        child: Obx(
          () =>con.isLoading.value?LoadingWidget():Column(
            children: [
              Container(
                padding: const EdgeInsets.all(10),
                margin: const EdgeInsets.all(10),
                height: 200,
                alignment: AlignmentDirectional.topStart,
                decoration: BoxDecoration(
                    borderRadius: BorderRadius.circular(10),
                    border: Border.all(color: Colors.grey.shade400, width: 1),
                    color: Colors.grey.shade100),
                child: SingleChildScrollView(
                  child: Wrap(
                    children: con.skillsList
                        .where(
                      (p0) => p0.isSelected,
                    )
                        .map(
                      (e) {
                        return Padding(
                          padding: const EdgeInsets.all(5),
                          child: ActionChip(
                            label: Wrap(
                              crossAxisAlignment: WrapCrossAlignment.center,
                              children: [
                                SubTxtWidget(
                                  e.title,
                                  color: Colors.white,
                                ),
                                const SizedBox(
                                  width: 5,
                                ),
                                const Icon(
                                  Icons.close,
                                  size: 15,
                                  color: Colors.white,
                                )
                              ],
                            ),
                            onPressed: () {
                              setState(() {
                                e.isSelected = false;
                              });
                            },
                            color: WidgetStatePropertyAll("#3348FF".toColor()),
                            shape: const ContinuousRectangleBorder(
                                borderRadius:
                                BorderRadius.all(Radius.circular(30)),
                                side: BorderSide(color: Colors.transparent)),
                          ),
                        );
                      },
                    ).toList(),
                  ),
                ),
              ),
              Expanded(child: SingleChildScrollView(child: Container(
                padding: const EdgeInsets.all(10),
                margin: const EdgeInsets.all(10),
                alignment: AlignmentDirectional.topStart,
                child: Wrap(
                  children: con.skillsList.map(
                        (e) {
                      return Padding(
                        padding: const EdgeInsets.all(5),
                        child: ActionChip(
                          label: SubTxtWidget(e.title,color: e.isSelected?Colors.white:Colors.black,),
                          onPressed: () {
                            setState(() {
                              e.isSelected = true;
                            });
                          },
                          color: WidgetStatePropertyAll(e.isSelected
                              ? "#3348FF".toColor()
                              : Colors.grey.shade200),
                          shape: const ContinuousRectangleBorder(
                              borderRadius:
                              BorderRadius.all(Radius.circular(30)),
                              side: BorderSide(color: Colors.transparent)),
                        ),
                      );
                    },
                  ).toList(),
                ),
              ),)),
              ButtonPrimaryWidget('Save',
              marginVertical: 20,
              marginHorizontal: 20,onTap: () async {
                await con.saveSkilled();
                Navigator.pop(context);
              },)
            ],
          ),
        ),
      ),
    );
  }
}
