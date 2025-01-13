import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../../utils/tools.dart';

enum ValidationType { EMAIL, TEXT, NONE, ShowError }

class InputWidget extends StatelessWidget {
  final TextEditingController? controller;
  final bool enabled;
  final bool obscureText;
  final inputType;
  final onChanged;
  final onDone;
  final Color? color;
  final Color? fillColor;
  final String? title;
  final String? sufix;
  final String? hint;
  final int? maxLength;
  final int? maxLines;
  final double? radius;
  final List<TextInputFormatter>? inputFormatters;
  final ValidationType validator;
  final Widget? suffixIcon;
  final Widget? prefixIcon;
  final Widget? child;
  final EdgeInsets? contentPadding;
  final EdgeInsets? margin;
  final String? errorMsg;
  final FormFieldValidator<String>? validatorCallback;
  final double? width;
  final TextStyle? hintStyle;
  // New parameter for hint text style

  InputWidget({
    super.key,
    this.controller,
    this.inputType = TextInputType.text,
    this.sufix,
    this.title,
    this.enabled = true,
    this.obscureText = false,
    this.color,
    this.fillColor,
    this.hint,
    this.maxLength,
    this.maxLines,
    this.validator = ValidationType.NONE,
    this.onChanged,
    this.onDone,
    this.inputFormatters,
    this.suffixIcon,
    this.child,
    this.radius,
    this.contentPadding,
    this.validatorCallback,
    this.margin,
    this.errorMsg,
    this.width,
    this.prefixIcon,
    this.hintStyle, // Initialize new parameter
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      width: width ?? MediaQuery.of(context).size.width,
      margin: margin ?? const EdgeInsets.symmetric(vertical: 10, horizontal: 5),
      alignment: AlignmentDirectional.topCenter,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          if (title != null && title!.isNotEmpty) ...{
            SubTxtWidget(
              fontWeight: FontWeight.w500,
              title!,
              fontSize: 12,
            ),
            const SizedBox(
              height: 10,
            ),
          },
          child ??
              TextFormField(
                obscureText: obscureText,
                enabled: enabled,
                controller: controller ?? TextEditingController(),
                keyboardType: inputType,
                inputFormatters: inputFormatters ??
                    [FilteringTextInputFormatter.singleLineFormatter],
                maxLines: obscureText ? 1 : maxLines ?? 1,
                maxLength: maxLength,
                onChanged: onChanged,
                onFieldSubmitted: onDone,
                validator: validatorCallback ??
                    (value) {
                      if (validator == ValidationType.NONE) {
                        return null;
                      }
                      if (validator == ValidationType.TEXT && value!.isEmpty) {
                        return errorMsg;
                      }
                      if (validator == ValidationType.EMAIL) {
                        if (value!.isEmpty) {
                          return errorMsg;
                        }
                        if (!Tools.isEmailValid(value.toString())) {
                          return "Please enter valid email Address";
                        }
                        return null;
                      }
                      if (validator == ValidationType.ShowError) {
                        return errorMsg;
                      }
                      return null;
                    },
                decoration: InputDecoration(
                  enabledBorder: OutlineInputBorder(
                    borderSide:
                        BorderSide(width: 1, color: color ?? primaryColorCode),
                    borderRadius: BorderRadius.circular(radius ?? 12.0),
                  ),
                  fillColor: fillColor ?? Colors.white,
                  contentPadding: contentPadding ??
                      const EdgeInsets.symmetric(horizontal: 10, vertical: 15),
                  focusedBorder: OutlineInputBorder(
                    borderSide:
                        BorderSide(width: 1, color: color ?? primaryColorCode),
                    borderRadius: BorderRadius.circular(radius ?? 12.0),
                  ),
                  errorBorder: OutlineInputBorder(
                    borderSide:
                        BorderSide(width: 1, color: color ?? Colors.red),
                    borderRadius: BorderRadius.circular(radius ?? 12.0),
                  ),
                  errorStyle: const TextStyle(color: Colors.red),
                  border: OutlineInputBorder(
                    borderSide:
                        BorderSide(width: 1, color: color ?? primaryColorCode),
                    borderRadius: BorderRadius.circular(radius ?? 12.0),
                  ),
                  focusedErrorBorder: OutlineInputBorder(
                    borderSide:
                        BorderSide(width: 1, color: color ?? Colors.red),
                    borderRadius: BorderRadius.circular(radius ?? 12.0),
                  ),
                  filled: true,
                  suffixText: sufix,
                  suffixStyle: const TextStyle(
                    color: Colors.black,
                  ),
                  hoverColor: Colors.transparent,
                  hintText: hint ?? "",
                  hintStyle: hintStyle ??
                      const TextStyle(
                        // Use the hintStyle parameter
                        overflow: TextOverflow.ellipsis,
                        fontWeight: FontWeight.w500,
                      ),
                  counterText: "",
                  suffixIcon: suffixIcon,
                  prefixIcon: prefixIcon,
                ),
              )
        ],
      ),
    );
  }
}
