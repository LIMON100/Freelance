//
// import 'package:flutter/material.dart';
// import 'package:google_fonts/google_fonts.dart';
//
// import '../utils/colors.dart';
//
// class CardWidget extends StatefulWidget {
//   String title;
//   String subTitle;
//   String icon;
//   CardWidget(
//       {super.key,
//         required this.title,
//         required this.icon,
//         required this.subTitle});
//
//   @override
//   State<CardWidget> createState() => _CardWidgetState();
// }
//
// class _CardWidgetState extends State<CardWidget> {
//   @override
//   Widget build(BuildContext context) {
//     return Container(
//       width: 172,
//       height: 90,
//       child: Card(
//         elevation: 0,
//         color: colorwhite,
//         shape: RoundedRectangleBorder(
//           side: BorderSide(
//             color: borderColor,
//           ),
//           borderRadius: BorderRadius.circular(5.0),
//         ),
//         child: ListTile(
//           title: Text(
//             widget.title,
//             style: GoogleFonts.workSans(
//                 color: mainBtnColor, fontSize: 22, fontWeight: FontWeight.w600),
//           ),
//           subtitle: Text(
//             widget.subTitle,
//             style: GoogleFonts.workSans(
//                 color: textformColor,
//                 fontSize: 12,
//                 fontWeight: FontWeight.w600),
//           ),
//           trailing: Image.asset(
//             widget.icon,
//             height: 30,
//             width: 30,
//           ),
//         ),
//       ),
//     );
//   }
// }

import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

import '../utils/colors.dart';

class CardWidget extends StatefulWidget {
  String title;
  String subTitle;
  String icon;
  CardWidget({
    super.key,
    required this.title,
    required this.icon,
    required this.subTitle,
  });

  @override
  State<CardWidget> createState() => _CardWidgetState();
}

class _CardWidgetState extends State<CardWidget> {
  @override
  Widget build(BuildContext context) {
    return Container(
      width: 172,
      height: 90,
      child: Card(
        elevation: 0,
        color: colorwhite,
        shape: RoundedRectangleBorder(
          side: BorderSide(
            color: borderColor,
          ),
          borderRadius: BorderRadius.circular(5.0),
        ),
        child: Padding(
          padding: const EdgeInsets.all(10),
          child: Row(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Text(
                      widget.title,
                      style: GoogleFonts.workSans(
                        color: mainBtnColor,
                        fontSize: 22,
                        fontWeight: FontWeight.w600,
                      ),
                      maxLines: 1,
                      overflow: TextOverflow.ellipsis,
                    ),
                    Text(
                      widget.subTitle,
                      style: GoogleFonts.workSans(
                        color: textformColor,
                        fontSize: 12,
                        fontWeight: FontWeight.w600,
                      ),
                      maxLines: 1,
                      overflow: TextOverflow.ellipsis,
                    ),
                  ],
                ),
              ),
              Image.asset(
                widget.icon,
                height: 30,
                width: 30,
              ),
            ],
          ),
        ),
      ),
    );
  }
}