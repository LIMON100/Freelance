import 'package:cached_network_image/cached_network_image.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';

import '../data_models/chat_info_model.dart';
import '../firebase/chat_firebase.dart';
import '../firebase/user_firebase.dart';
import 'chat_screen.dart';

class ChatListScreen extends StatefulWidget {
  final id;
  ChatListScreen({super.key, this.id});

  @override
  State<ChatListScreen> createState() => _ChatListScreenState();
}

class _ChatListScreenState extends State<ChatListScreen> {
  final GlobalKey<ScaffoldState> _scaffoldKey = GlobalKey();
  final UserFirebase firebaseService = UserFirebase();

  @override
  void initState() {
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return SafeArea(
      child: Scaffold(
        key: _scaffoldKey,
        appBar: AppBar(
          title: Text(
            "Messages",
            style: TextStyle(
                fontSize: 25,
                color: Color(0xFFFF6222),
                fontWeight: FontWeight.w600),
          ),
        ),
        body: StreamBuilder<List<ChatInfo>>(
          stream: getChatListByUserId(widget.id),
          builder: (context, snapshot) {
            if (snapshot.connectionState == ConnectionState.waiting) {
              return const Center(
                child: CircularProgressIndicator(),
              );
            } else if (snapshot.hasError) {
              return Text("Error: ${snapshot.error}");
            } else if (snapshot.hasData) {
              var chats = snapshot.data!;
              if (chats.isNotEmpty) {
                return Column(
                  children: [
                    Expanded(
                      child: ListView.builder(
                        itemCount: chats.length,
                        itemBuilder: (context, index) {
                          var chat = chats[index];
                          var imageurls =
                              firebaseService.getImage(chat.friendId);
                          print(imageurls);
                          return ClipRect(
                            clipBehavior: Clip.hardEdge,
                            child: GestureDetector(
                              onTap: () async {
                                Get.to(
                                  () => ChatDetailScreen(
                                    userId: widget.id,
                                    friendId: chat.createId,
                                  ),
                                );
                              },
                              child: Column(
                                children: [
                                  Divider(thickness: 2),
                                  Container(
                                    color: Colors.white,
                                    margin: const EdgeInsets.symmetric(
                                        vertical: 10, horizontal: 20),
                                    child: Row(
                                      children: [
                                        CircleAvatar(
                                          backgroundColor: Colors.transparent,
                                          radius: 30,
                                          child: CachedNetworkImage(
                                              imageUrl: chat.friendId !=
                                                      FirebaseAuth.instance
                                                          .currentUser!.uid
                                                  ? chat.friendImage
                                                  : chat.createdImage),
                                        ),
                                        Padding(
                                          padding: const EdgeInsets.all(3.0),
                                          child: Column(
                                            crossAxisAlignment:
                                                CrossAxisAlignment.start,
                                            children: [
                                              Padding(
                                                padding:
                                                    const EdgeInsets.all(2.0),
                                                child: FirebaseAuth.instance
                                                            .currentUser!.uid !=
                                                        chat.friendId
                                                    ? Text(
                                                        " ${chat.friendName.capitalizeFirst}  ",
                                                        style: TextStyle(
                                                            fontSize: 12),
                                                      )
                                                    : Text(
                                                        "${chat.createName.capitalizeFirst}",
                                                        style: TextStyle(
                                                            fontSize: 12),
                                                      ),
                                              ),
                                              Padding(
                                                padding:
                                                    const EdgeInsets.all(3.0),
                                                child: Container(
                                                  width: MediaQuery.of(context)
                                                          .size
                                                          .width *
                                                      0.5,
                                                  height: MediaQuery.of(context)
                                                          .size
                                                          .height *
                                                      0.04,
                                                  child: Text(
                                                    " ${chat.lastMessage}",
                                                    style: const TextStyle(
                                                        fontSize: 18),
                                                  ),
                                                ),
                                              ),
                                            ],
                                          ),
                                        ),
                                      ],
                                    ),
                                  ),
                                ],
                              ),
                            ),
                          );
                        },
                      ),
                    ),
                  ],
                );
              } else {
                return const Center(child: Text("No data found."));
              }
            } else {
              return const Center(child: Text("No data found!"));
            }
          },
        ),
      ),
    );
  }
}
