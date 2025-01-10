import 'package:flutter/material.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../utils/colors.dart';
import 'button_primary_widget.dart';
import 'header_txt_widget.dart';

class ConfirmDailogWidget extends StatelessWidget {
  String title;
  String sub_title;
  String? positive_button_text;
  String? negative_button_text;
  bool showCancelButton;
  Function()? onSuccess;
  ConfirmDailogWidget(
      {required this.title,
      required this.sub_title,
      this.onSuccess,
      this.positive_button_text,
      this.negative_button_text,
      this.showCancelButton = true});

  @override
  Widget build(BuildContext context) {
    return Material(
      color: Colors.black38,
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Container(
            margin: const EdgeInsets.symmetric(horizontal: 30),
            decoration: BoxDecoration(
                borderRadius: const BorderRadius.all(Radius.circular(10)),
                color: Theme.of(context).scaffoldBackgroundColor,
                boxShadow: [
                  BoxShadow(color: Colors.grey.shade300, blurRadius: 5)
                ]),
            padding: EdgeInsets.all(15),
            child: Column(
              children: [
                HeaderTxtWidget(title),
                const SizedBox(
                  height: 10,
                ),
                SubTxtWidget(sub_title),
                const SizedBox(
                  height: 10,
                ),
                Row(
                  children: [
                    Expanded(
                      flex: 1,
                      child: ButtonPrimaryWidget(
                        positive_button_text ?? 'OK',
                        fontSize: 14,
                        height: 40,
                        padding: 0,
                        onTap: () {
                          Navigator.pop(context);
                          if (onSuccess != null) {
                            onSuccess!.call();
                          }
                        },
                      ),
                    ),
                    if (showCancelButton)
                      Expanded(
                        flex: 1,
                        child: ButtonPrimaryWidget(
                          negative_button_text ?? 'Cancel',
                          fontSize: 14,
                          height: 40,
                          padding: 0,
                          color: Colors.white,
                          borderColor: primaryColorCode,
                          txtColor: primaryColorCode,
                          onTap: () {
                            Navigator.pop(context);
                          },
                        ),
                      ),
                  ],
                )
              ],
            ),
          ),
        ],
      ),
    );
  }
}
