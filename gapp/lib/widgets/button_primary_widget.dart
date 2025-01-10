import 'package:flutter/material.dart';

typedef callback = Function();

class ButtonPrimaryWidget extends StatelessWidget {
  String title;
  callback? onTap;
  double fontSize;
  Widget? leading;
  Widget? trailing;
  Color? color;
  Color? txtColor;
  double padding;
  double marginVertical;
  double marginHorizontal;
  double? radius;
  double? width;
  double? height;
  Color? borderColor;
  bool isLoading;
  FontWeight? fontWeight;
  ButtonPrimaryWidget(this.title,
      {this.fontSize = 16,
      this.onTap,
      this.leading,
      this.trailing,
      this.color,
      this.txtColor,
      this.padding = 13,
      this.marginVertical = 5,
      this.marginHorizontal = 0,
      this.radius,
      this.width,
      this.borderColor,
      this.isLoading=false,
      this.height,
        this.fontWeight
      });

  @override
  Widget build(BuildContext context) {
    double wid = MediaQuery.of(context).size.width;
    color=color??Theme.of(context).primaryColor;
    return Container(
      alignment: AlignmentDirectional.center,
      child: InkWell(
        onTap: onTap,
        splashColor: Colors.transparent,
        hoverColor: Colors.transparent,
        highlightColor: Colors.transparent,
        child: AnimatedContainer(
          duration: const Duration(milliseconds: 500),
          width:isLoading?54: width ?? wid,
          height: height?? 54,
          alignment: AlignmentDirectional.center,
          padding: EdgeInsets.all(padding),
          margin: EdgeInsets.symmetric(vertical: marginVertical, horizontal: marginHorizontal),
          decoration: BoxDecoration(
              color: color,
              border: Border.all(
                color: borderColor??color??Colors.white,
              ),
              borderRadius:  BorderRadius.all(Radius.circular(isLoading?30:radius??15))
          ),
          child:isLoading?const SizedBox(
            height: 30,width: 30,
            child: CircularProgressIndicator(
              color: Colors.white,
            ),
          ):Stack(
            children: [
              if (leading != null)...{
                Positioned(
                  left: 10,
                  child: leading!,
                )
              },
              Center(
                  child: Text(
                    title,
                    textAlign: TextAlign.center,
                    style: TextStyle(
                        fontSize: fontSize,
                        fontStyle: FontStyle.normal,
                        fontWeight: fontWeight??FontWeight.normal,
                        color: txtColor??Colors.white),
                  )),
              if (trailing != null)...{
                Positioned(
                  right: 10,
                  bottom: 0,
                  top: 0,
                  child: Center(child: trailing!,),
                )
              },
            ],
          ),
        ),
      ),
    );
  }
}
