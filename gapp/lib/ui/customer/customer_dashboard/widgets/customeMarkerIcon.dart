import 'dart:typed_data';
import 'dart:ui' as ui;
import 'dart:io';
import 'package:cached_network_image/cached_network_image.dart';
import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:groom/constant/asset.dart';

enum ImageSourceType { network, asset, file }

class CustomMarkerIcon extends StatefulWidget {
  final String text;
  final markerid;
  final selectedcatgory;
  final ismarkeractive;
  final iscategoryseleted;
  final String imageUrl;
  final ImageSourceType imageSourceType;

  const CustomMarkerIcon({
    super.key,
    required this.text,
    required this.imageUrl,
    required this.imageSourceType,
    this.iscategoryseleted,
    this.ismarkeractive,
    this.selectedcatgory,
    this.markerid,
  });

  @override
  State<CustomMarkerIcon> createState() => _CustomMarkerIconState();
}

class _CustomMarkerIconState extends State<CustomMarkerIcon> {
  @override
  Widget build(BuildContext context) {
    return Stack(
      alignment: Alignment.center,
      children: [
        SizedBox(
          height:
              widget.iscategoryseleted == false || widget.ismarkeractive == true
                  ? 100
                  : 60,
          width:
              widget.iscategoryseleted == false || widget.ismarkeractive == true
                  ? 80
                  : 60,
          child: widget.iscategoryseleted == false ||
                  widget.ismarkeractive == true
              ? SvgPicture.asset(AssetConstants.supporticon, fit: BoxFit.fill)
              : SvgPicture.asset(AssetConstants.supporticon, fit: BoxFit.fill),
        ),
        Padding(
          padding: EdgeInsets.only(bottom: 12.5),
          child: SizedBox(
            height: widget.iscategoryseleted == false ||
                    widget.ismarkeractive == true
                ? 75
                : 30,
            width: widget.iscategoryseleted == false ||
                    widget.ismarkeractive == true
                ? 75
                : 30,
            child: CachedNetworkImage(
              imageUrl: widget.imageUrl,
              imageBuilder: (context, imageProvider) => Container(
                decoration: BoxDecoration(
                  borderRadius: BorderRadius.circular(15),
                  image: DecorationImage(
                    image: imageProvider,
                    fit: BoxFit
                        .contain, // Ensures the image fits inside the container
                  ),
                ),
              ),
              errorWidget: (context, url, error) => SizedBox(
                height: widget.iscategoryseleted == false ||
                        widget.ismarkeractive == true
                    ? 75
                    : 30,
                width: widget.iscategoryseleted == false ||
                        widget.ismarkeractive == true
                    ? 75
                    : 30,
                child: Image.asset("assets/logo.png"),
              ), // Fallback for errors
              placeholder: (context, url) => SizedBox(
                height: widget.iscategoryseleted == false ||
                        widget.ismarkeractive == true
                    ? 75
                    : 30,
                width: widget.iscategoryseleted == false ||
                        widget.ismarkeractive == true
                    ? 75
                    : 30,
                child: Image.asset(
                    "assets/logo.png"), // Optional loading indicator
              ),
            ),
          ),
        ),
        widget.text == "0" || widget.text == "1"
            ? Container()
            : Positioned(
                bottom: widget.ismarkeractive == true
                    ? 80
                    : 70, // Change the position as needed
                left: widget.ismarkeractive == true
                    ? 70
                    : 70, // Change the position as needed
                child: Container(
                  padding: EdgeInsets.all(4),
                  decoration: BoxDecoration(
                    color: Colors.grey,
                    shape: BoxShape.circle,
                  ),
                  child: Text(
                    "+${widget.text}",
                    style: TextStyle(
                      color: Colors.white,
                      fontSize: 8,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ),
              ),
      ],
    );
  }

  Widget _buildImage() {
    switch (widget.imageSourceType) {
      case ImageSourceType.network:
        return CachedNetworkImage(
          imageUrl: widget.imageUrl,
          imageBuilder: (context, imageProvider) => Container(
            decoration: BoxDecoration(
              borderRadius: BorderRadius.circular(15),
              image: DecorationImage(
                image: imageProvider,
                fit: BoxFit
                    .contain, // Ensures the image fits inside the container
              ),
            ),
          ),
          errorWidget: (context, url, error) =>
              Image.asset("assets/logo.png"), // Fallback for errors
          placeholder: (context, url) => SizedBox(
            height: widget.iscategoryseleted == false ? 50 : 30,
            width: widget.iscategoryseleted == false ? 50 : 30,
            child: Image.asset("assets/logo.png"), // Optional loading indicator
          ),
        );
      case ImageSourceType.asset:
        return Image.asset(
          widget.imageUrl,
          fit: BoxFit.contain,
          errorBuilder:
              (BuildContext context, Object error, StackTrace? stackTrace) {
            return Image.asset(
              "assets/images/icon.png",
              fit: BoxFit.contain,
            ); // Fallback for errors
          },
        );
      case ImageSourceType.file:
        return Image.file(
          File(widget.imageUrl),
          fit: BoxFit.contain,
          errorBuilder:
              (BuildContext context, Object error, StackTrace? stackTrace) {
            return Image.asset("assets/logo.png"); // Fallback for errors
          },
        );
      default:
        return Image.asset("assets/logo.png"); // Default fallback
    }
  }
}

extension ToBitDescription on Widget {
  Future<BitmapDescriptor> toBitmapDescriptor({
    Size? logicalSize,
    Size? imageSize,
    Duration waitToRender = const Duration(milliseconds: 300),
    TextDirection textDirection = TextDirection.ltr,
  }) async {
    final widget = RepaintBoundary(
      child: SizedBox(
        width: logicalSize?.width ?? 100,
        height: logicalSize?.height ?? 100,
        child: MediaQuery(
          data: const MediaQueryData(),
          child: Directionality(
            textDirection: textDirection,
            child: this,
          ),
        ),
      ),
    );

    final pngBytes = await createImageFromWidget(
      widget,
      waitToRender: waitToRender,
      logicalSize: logicalSize,
      imageSize: imageSize,
    );
    return BitmapDescriptor.fromBytes(pngBytes);
  }
}

Future<Uint8List> createImageFromWidget(
  Widget widget, {
  Size? logicalSize,
  required Duration waitToRender,
  Size? imageSize,
}) async {
  final RenderRepaintBoundary repaintBoundary = RenderRepaintBoundary();
  final view = ui.PlatformDispatcher.instance.views.first;

  logicalSize ??= view.physicalSize / view.devicePixelRatio;
  imageSize ??= view.physicalSize;

  if (logicalSize.width <= 0 ||
      logicalSize.height <= 0 ||
      imageSize.width <= 0 ||
      imageSize.height <= 0) {
    throw Exception('Invalid image dimensions: Width or height is zero.');
  }

  final RenderView renderView = RenderView(
    view: view,
    child: RenderPositionedBox(
      alignment: Alignment.center,
      child: repaintBoundary,
    ),
    configuration: ViewConfiguration(
      physicalConstraints: BoxConstraints.tight(imageSize),
      logicalConstraints: BoxConstraints.tight(logicalSize),
      devicePixelRatio: view.devicePixelRatio,
    ),
  );

  final PipelineOwner pipelineOwner = PipelineOwner();
  final BuildOwner buildOwner = BuildOwner(focusManager: FocusManager());

  pipelineOwner.rootNode = renderView;
  renderView.prepareInitialFrame();

  final RenderObjectToWidgetElement<RenderBox> rootElement =
      RenderObjectToWidgetAdapter<RenderBox>(
    container: repaintBoundary,
    child: widget,
  ).attachToRenderTree(buildOwner);

  buildOwner.buildScope(rootElement);
  await Future.delayed(waitToRender);
  buildOwner.buildScope(rootElement);
  buildOwner.finalizeTree();

  pipelineOwner.flushLayout();
  pipelineOwner.flushCompositingBits();
  pipelineOwner.flushPaint();

  final ui.Image image = await repaintBoundary.toImage(
    pixelRatio: imageSize.width / logicalSize.width,
  );
  final ByteData? byteData =
      await image.toByteData(format: ui.ImageByteFormat.png);

  if (byteData == null) {
    throw Exception("Failed to convert image to bytes");
  }

  return byteData.buffer.asUint8List();
}
