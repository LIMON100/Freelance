import 'package:fast_contacts/fast_contacts.dart';
import 'package:flutter/material.dart';
import 'package:flutter_libphonenumber/flutter_libphonenumber.dart';
import 'package:get/get.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/loading_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import 'invite_controller.dart';

class InvitePage extends StatefulWidget {
  const InvitePage({super.key});

  @override
  State<InvitePage> createState() => _ScreenState();
}

class _ScreenState extends State<InvitePage> {
  final con = Get.put(InviteController());

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      appBar: AppBar(
        backgroundColor: Colors.white,
        iconTheme: IconThemeData(color: primaryColorCode),
        title: Obx(
          () => Container(
            child: con.showSearch.value
                ? TextField(
                    decoration:
                        const InputDecoration(hintText: "Search by name"),
                    onChanged: (value) {
                      con.searchContacts(value.toString());
                    },
                  )
                : HeaderTxtWidget('Invite Friends'),
          ),
        ),
        actions: [
          Obx(
            () => Row(
              children: [
                IconButton(
                    onPressed: () {
                      con.showSearch.value = !con.showSearch.value;
                      if (!con.showSearch.value) {
                        con.searchContacts("");
                      }
                    },
                    icon: Icon(
                        con.showSearch.value ? Icons.close : Icons.search)),
                if (!con.showSearch.value)
                  InkWell(
                    onTap: () {
                      if (con.showSelectAll.value) {
                        setState(() {
                          con.isSelectAll.value = !con.isSelectAll.value;
                          if (con.isSelectAll.value) {
                            con.selectedContactList.addAll(con.contactList.map(
                              (element) => element.phones.first.number,
                            ));
                          } else {
                            con.selectedContactList.clear();
                          }
                        });
                      } else {
                        con.showSelectAll.value = true;
                      }
                    },
                    child: Container(
                      padding: const EdgeInsets.symmetric(
                          horizontal: 10, vertical: 5),
                      decoration: BoxDecoration(
                          borderRadius: BorderRadius.circular(20),
                          color: primaryColorCode),
                      margin: const EdgeInsets.symmetric(horizontal: 10),
                      child: con.showSelectAll.value
                          ? Row(
                              children: [
                                Icon(
                                  con.isSelectAll.value
                                      ? Icons.check_box_outlined
                                      : Icons.check_box_outline_blank,
                                  color: Colors.white,
                                  size: 18,
                                ),
                                const SizedBox(
                                  width: 5,
                                ),
                                SubTxtWidget(
                                  'Select all',
                                  color: Colors.white,
                                  fontSize: 14,
                                )
                              ],
                            )
                          : Row(
                              children: [
                                const Icon(
                                  Icons.add,
                                  color: Colors.white,
                                ),
                                const SizedBox(
                                  width: 5,
                                ),
                                SubTxtWidget(
                                  'Select',
                                  color: Colors.white,
                                  fontSize: 14,
                                )
                              ],
                            ),
                    ),
                  )
              ],
            ),
          ),
        ],
      ),
      body: Obx(
        () => Column(
          children: [
            Expanded(child: _body()),
            if (con.showSelectAll.value)
              Container(
                padding: const EdgeInsets.only(bottom: 10, top: 10),
                decoration: const BoxDecoration(color: Colors.white),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  children: [
                    ActionChip(
                      label: SubTxtWidget(
                        'Invite Selected',
                        fontSize: 14,
                        color: Colors.white,
                      ),
                      shape: const StadiumBorder(
                        side: BorderSide(color: Colors.white),
                      ),
                      color: WidgetStatePropertyAll(primaryColorCode),
                      onPressed: () {
                        con.sendInviteAll();
                      },
                    ),
                    ActionChip(
                      label: SubTxtWidget(
                        'Cancel',
                        fontSize: 14,
                      ),
                      shape: const StadiumBorder(
                        side: BorderSide(color: Colors.white),
                      ),
                      color: WidgetStatePropertyAll(Colors.grey.shade100),
                      onPressed: () {
                        con.showSelectAll.value = false;
                        con.isSelectAll.value = false;
                        con.selectedContactList.clear();
                      },
                    )
                  ],
                ),
              )
          ],
        ),
      ),
    );
  }

  Widget _body() {
    if (con.isLoading.value) {
      return LoadingWidget();
    }
    if (con.contactList.isEmpty) {
      return Container(
        alignment: AlignmentDirectional.center,
        padding: const EdgeInsets.all(50),
        child: SubTxtWidget("No Contact found"),
      );
    }
    return ListView.builder(
      itemBuilder: (context, index) {
        return _tile(con.contactList[index]);
      },
      itemCount: con.contactList.length,
      shrinkWrap: true,
    );
  }

  Widget _tile(Contact data) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 10),
      child: Row(
        children: [
          FutureBuilder(
            future: FastContacts.getContactImage(data.id),
            builder: (context, snapshot) {
              return ClipRRect(
                borderRadius: BorderRadius.circular(30),
                child: snapshot.hasData
                    ? Image.memory(
                        snapshot.data!,
                        gaplessPlayback: true,
                        width: 50,
                        height: 50,
                      )
                    : Icon(
                        Icons.account_circle,
                        size: 50,
                        color: Colors.grey.shade400,
                      ),
              );
            },
          ),
          const SizedBox(
            width: 10,
          ),
          Expanded(
              child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              HeaderTxtWidget(data.displayName),
              SubTxtWidget(con.addCountryCodeIfMissing(data.phones.first.number,context)),
            ],
          )),
          _btn(data),
        ],
      ),
    );
  }

  Widget _btn(Contact data) {
    if (con.showSelectAll.value) {
      bool isSelected =
          con.selectedContactList.contains(data.phones.first.number);
      return IconButton(
          onPressed: () {
            setState(() {
              if (isSelected) {
                con.isSelectAll.value = false;
                con.selectedContactList.remove(data.phones.first.number);
              } else {
                con.selectedContactList.add(data.phones.first.number);
              }
            });
          },
          icon: isSelected
              ? const Icon(Icons.check_box_outlined)
              : const Icon(Icons.check_box_outline_blank));
    }
    return ActionChip(
      label: SubTxtWidget(
        'Invite',
        fontSize: 12,
      ),
      shape: const StadiumBorder(
        side: BorderSide(color: Colors.white),
      ),
      color: WidgetStatePropertyAll(Colors.grey.shade100),
      onPressed: () {
        con.sendInvite(phone: data.phones.first.number);
      },
    );
  }
}
