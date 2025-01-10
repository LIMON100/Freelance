import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/states/provider_state.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';

class CardListSwap extends StatefulWidget {
  @override
  _CardListSwapState createState() => _CardListSwapState();
}

class _CardListSwapState extends State<CardListSwap> {
  List<String> list1 = ["HairCut", "Nail-Care", "Coloring", "Wax", "Spa", "Massage", "Facial", "MakeUp"];
  List<String> list2 = [];
  final formController = Get.find<FormController>();

  void moveCard(List<String> fromList, List<String> toList, String item) {
    setState(() {
      fromList.remove(item);
      toList.add(item);
    });
    if (fromList == list1) {
      formController.providerUser.update((user) {
        user?.providerServices.add(item);
      });
    } else {
      formController.providerUser.update((user) {
        user?.providerServices.remove(item);
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        HeaderTxtWidget("Choose from the following services:"),
        const SizedBox(height: 10),
        Container(
          width: MediaQuery.of(context).size.width,
          decoration: BoxDecoration(border: Border.all(color: Colors.blueGrey),
          borderRadius: const BorderRadius.all(Radius.circular(12))),
          padding: const EdgeInsets.all(10),
          child: Wrap(
            spacing: 8.0, // gap between adjacent cards
            runSpacing: 8.0, // gap between lines
            children: list1.map((item) {
              return GestureDetector(
                onTap: () => moveCard(list1, list2, item),
                child: Card(
                  child: Padding(
                    padding: const EdgeInsets.all(10.0),
                    child: Text(item),
                  ),
                ),
              );
            }).toList(),
          ),
        ),
        SizedBox(height: 10),
        SubTxtWidget("Grooming services"),
        SizedBox(height: 10),
        Container(
          width: MediaQuery.of(context).size.width,
          decoration: BoxDecoration(border: Border.all(color: Colors.blueGrey),
          borderRadius: BorderRadius.circular(12)),
          child: Wrap(
            spacing: 8.0, // gap between adjacent cards
            runSpacing: 8.0, // gap between lines
            children: list2.map((item) {
              return GestureDetector(
                onTap: () => moveCard(list2, list1, item),
                child: Card(
                  child: Padding(
                    padding: const EdgeInsets.all(7.0),
                    child: Text(item),
                  ),
                ),
              );
            }).toList(),
          ),
        ),
      ],
    );
  }
}
