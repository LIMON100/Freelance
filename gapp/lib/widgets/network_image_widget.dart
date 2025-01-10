import 'package:cached_network_image/cached_network_image.dart';
import 'package:flutter/material.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/generated/assets.dart';
import 'package:lottie/lottie.dart';

class NetworkImageWidget extends StatelessWidget {
  String url;
  double? width;
  double? height;
  BoxFit fit;
  Function()? onTap;
  NetworkImageWidget(
      {required this.url,
      this.width,
      this.height,
      this.fit = BoxFit.fill,
      this.onTap});

  @override
  Widget build(BuildContext context) {
    double wid = width ?? MediaQuery.of(context).size.width;
    return Container(
      child: url.isEmpty
          ? Container(
              color: "#F2F2F2".toColor(),
              child: Image.asset(
                Assets.imgNoImage,
                width: wid,
                fit: BoxFit.contain,
                height: height,
              ),
            )
          : InkWell(
              onTap: onTap,
              child: CachedNetworkImage(
                imageUrl: url,
                placeholder: (context, url) => LottieBuilder.asset(
                  'assets/json/image_loading.json',
                  width: wid,
                  fit: BoxFit.contain,
                  height: height,
                ),
                errorWidget: (context, url, error) => Container(
                  color: "#F2F2F2".toColor(),
                  child: Image.asset(
                    Assets.imgNoImage,
                    width: wid,
                    fit: BoxFit.contain,
                    height: height,
                  ),
                ),
                fit: fit,
                height: height,
                width: wid,
              ),
            ),
    );
  }
}
