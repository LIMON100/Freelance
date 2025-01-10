import 'package:flutter/material.dart';
import 'package:groom/widgets/sub_txt_widget.dart';

import '../utils/colors.dart';

class CustomSwitch extends StatefulWidget {
  final bool value;
  final bool showText;
  final Color borderColor;
  final Color innerColor;
  String positiveText;
  String negativeText;
  final ValueChanged<bool> onChanged;

  CustomSwitch(
      {Key? key,
      required this.value,
      required this.onChanged,
      this.showText = false,
      this.positiveText = "On",
      this.negativeText = "Off",
      required this.borderColor,
      required this.innerColor})
      : super(key: key);

  @override
  _CustomSwitchState createState() => _CustomSwitchState();
}

class _CustomSwitchState extends State<CustomSwitch>
    with SingleTickerProviderStateMixin {
  AnimationController? _animationController;

  @override
  void initState() {
    super.initState();
    _animationController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 60),
    );
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: _animationController!,
      builder: (context, child) {
        return GestureDetector(
          onTap: () {
            if (_animationController!.isCompleted) {
              _animationController!.reverse();
            } else {
              _animationController!.forward();
            }
            widget.value == false
                ? widget.onChanged(true)
                : widget.onChanged(false);
          },
          child: Container(
            width: 55.0,
            height: 28.0,
            decoration: BoxDecoration(
              borderRadius: BorderRadius.circular(24.0),
              border: Border.all(color: widget.borderColor, width: 1),
              color: widget.value ? widget.innerColor : Colors.white,
            ),
            child: Padding(
              padding: const EdgeInsets.only(
                  top: 2.0, bottom: 2.0, right: 2.0, left: 2.0),
              child: Row(
                mainAxisAlignment: widget.value
                    ? MainAxisAlignment.end
                    : MainAxisAlignment.start,
                children: [
                  if (widget.value && widget.showText)
                    SubTxtWidget(
                      "${widget.positiveText} ",
                      fontSize: 12,
                      color: Colors.white,
                    ),
                  Container(
                    width: 25.0,
                    height: 25.0,
                    decoration: BoxDecoration(
                        shape: BoxShape.circle,
                        color: widget.value ? Colors.white : primaryColorCode),
                  ),
                  if (!widget.value && widget.showText)
                    SubTxtWidget(
                      widget.negativeText,
                      fontSize: 12,
                    ),
                ],
              ),
            ),
          ),
        );
      },
    );
  }
}
