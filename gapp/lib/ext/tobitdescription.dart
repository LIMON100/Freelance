import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'dart:ui' as ui;
extension ToBitDescription on Widget {
  Future<BitmapDescriptor> toBitmapDescriptor(
      {Size? logicalSize,
        Size? imageSize,
        Duration waitToRender = const Duration(seconds: 2),
        TextDirection textDirection = TextDirection.ltr}) async {
    final widget = RepaintBoundary(
      child: MediaQuery(
          data: const MediaQueryData(),
          child: Directionality(textDirection: TextDirection.ltr, child: this)),
    );

    final view = ui.PlatformDispatcher.instance.views.first;

    final pngBytes = await createImageFromWidget(widget,
        waitToRender: waitToRender,
        view: view,
        logicalSize: logicalSize,
        imageSize: imageSize);

    return BitmapDescriptor.bytes(pngBytes,
        imagePixelRatio: view.devicePixelRatio);
  }
  Future<Uint8List> createImageFromWidget(Widget widget,
      {Size? logicalSize,
        required Duration waitToRender,
        required ui.FlutterView view,
        Size? imageSize}) async {
    final RenderRepaintBoundary repaintBoundary = RenderRepaintBoundary();
    logicalSize ??= view.physicalSize / view.devicePixelRatio;
    imageSize ??= view.physicalSize;

    // assert(logicalSize.aspectRatio == imageSize.aspectRatio);

    final RenderView renderView = RenderView(
      view: view,
      child: RenderPositionedBox(
          alignment: Alignment.center, child: repaintBoundary),
      configuration: ViewConfiguration(
        physicalConstraints:
        BoxConstraints.tight(logicalSize) * view.devicePixelRatio,
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
        pixelRatio: imageSize.width / logicalSize.width);
    final ByteData? byteData =
    await image.toByteData(format: ui.ImageByteFormat.png);

    return byteData!.buffer.asUint8List();
  }
}