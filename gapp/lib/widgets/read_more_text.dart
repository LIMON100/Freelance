import 'package:flutter/material.dart';
import 'package:groom/widgets/sub_txt_widget.dart';

class ReadMoreText extends StatefulWidget {
  const ReadMoreText(
    this.data, {
    Key? key,
  }) : super(key: key);

  final String data;

  @override
  ReadMoreTextState createState() => ReadMoreTextState();
}

class ReadMoreTextState extends State<ReadMoreText> {
  bool _readMore = false;

  void _onTapLink() {
    setState(() {
      _readMore = !_readMore;
    });
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedContainer(
      duration: const Duration(seconds: 1),
      child: Wrap(
        children: [
          if (widget.data.isNotEmpty)
            if (widget.data.length > 150) ...{
              _readMore
                  ? SubTxtWidget(widget.data)
                  : SubTxtWidget(widget.data.substring(0, 150)),
              InkWell(
                  child: SubTxtWidget(
                    _readMore ? "Show Less" : "Show more",
                    fontSize: 12,
                    color: Colors.blue,
                  ),
                  onTap: () {
                    _onTapLink();
                  }),
            } else
              SubTxtWidget(
                widget.data, fontSize: 16.0, // Font size in logical pixels
                fontWeight: FontWeight.w500,
              ),
        ],
      ),
    );
  }
}
