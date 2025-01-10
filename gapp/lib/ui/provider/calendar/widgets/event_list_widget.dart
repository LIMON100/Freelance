import 'package:calendar_view/calendar_view.dart';
import 'package:flutter/material.dart';
import 'package:groom/Utils/tools.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';

class EventListWidget extends StatelessWidget {
  Function(dynamic data) onTapEvent;
  List<CalendarEventData<Object?>> events;
  EventListWidget({required this.onTapEvent,required this.events});

  @override
  Widget build(BuildContext context) {
    return Material(
      color: Colors.black38,
      child: Container(
        height: MediaQuery.of(context).size.height*0.8,
        alignment: AlignmentDirectional.center,
        child: Wrap(
          children: [
            Container(
              margin: const EdgeInsets.symmetric(horizontal: 30),
              decoration: BoxDecoration(
                  borderRadius: const BorderRadius.all(Radius.circular(10)),
                  color: Theme.of(context).scaffoldBackgroundColor,
                  boxShadow: [
                    BoxShadow(color: Colors.grey.shade300, blurRadius: 5)
                  ]),
              padding: const EdgeInsets.all(10),
              child:Stack(
                children: [
                  Column(
                    children:events.map((e){
                      return ListTile(
                        title: Row(
                          children: [
                            HeaderTxtWidget(e.title),
                            const SizedBox(width: 10,),
                            SubTxtWidget(Tools.changeDateFormat(e.date.toString(), "MM-dd-yyyy"),fontSize: 12,)
                          ],
                        ),
                        subtitle: SubTxtWidget(e.description!),
                        onTap: () {
                          onTapEvent.call(e.event);
                        },
                      );
                    },).toList(),
                  ),
                Positioned(right: 10,top: 0,child: IconButton(onPressed: () {
                  Navigator.pop(context);
                }, icon: const Icon(Icons.close)),)
                ],
              )
             )
          ],
        ),
      ),
    );
  }
}
