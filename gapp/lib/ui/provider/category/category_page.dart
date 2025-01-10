import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';

import '../../../utils/colors.dart';
import '../../../widgets/button_primary_widget.dart';
import '../../../widgets/header_txt_widget.dart';
import '../../../widgets/loading_widget.dart';
import '../../../widgets/sub_txt_widget.dart';
import 'category_controller.dart';

class CategoryPage extends StatefulWidget {
  const CategoryPage({super.key});

  @override
  State<CategoryPage> createState() => _ScreenState();
}


class _ScreenState extends State<CategoryPage> {
  final con = Get.put(CategoryController());

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.white,
        title: HeaderTxtWidget('Category'),
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
                  await con.saveCategory();
                  Navigator.pop(context);
                },)
            ],
          ),
        ),
      ),
    );
  }
}
