

import 'package:firebase_auth/firebase_auth.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import '../constant/global_configuration.dart';
import '../data_models/chat_info_model.dart';
import '../data_models/message_model.dart';
import '../utils/utils.dart';

Future<bool> checkChatList(
    String currentUserId, String chatUserId, ChatInfo chatInfo) async {
  final DatabaseReference _databaseReference = FirebaseDatabase.instanceFor(
      app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL'))
      .ref();

  try {
    final snapshot = await _databaseReference.child("chatlist")
        .child(currentUserId)
        .child(chatUserId)
        .once();
    if (snapshot.snapshot.value != null) {
      await createChat(currentUserId, chatUserId, chatInfo);
    } else {
      await appendChat(currentUserId, chatUserId, chatInfo);
    }
    return false;
  } catch (error) {
    print('Error checking application: $error');
    return false;
  }
}

Future<bool> createChat(
    String currentUserId, String chatUserId, ChatInfo chatInfo) async {
  final DatabaseReference _databaseReference = FirebaseDatabase.instanceFor(
      app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL'))
      .ref();

  try {
    await _databaseReference.child("chatlist")
        .child(currentUserId)
        .child(chatUserId)
        .set(chatInfo.toJson());

    await _databaseReference.child("chatlist")
        .child(chatUserId)
        .child(currentUserId)
        .set(chatInfo.toJson());

    return true;
  } catch (e) {
    print(e);
    return false;
  }
}

Future<bool> appendChat(
    String currentUserId, String chatUserId, ChatInfo chatInfo) async {
  final DatabaseReference _databaseReference = FirebaseDatabase.instanceFor(
      app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL'))
      .ref();

  try {
    await _databaseReference.child("chatlist")
        .child(chatUserId)
        .child(currentUserId)
        .update(chatInfo.toJson());

    await _databaseReference.child("chatlist")
        .child(currentUserId)
        .child(chatUserId)
        .update(chatInfo.toJson());

    return true;
  } catch (e) {
    print(e);
    return false;
  }
}
Stream<List<ChatInfo>> getChatListByUserId(String userId) {
  final DatabaseReference _databaseReference = FirebaseDatabase.instanceFor(
      app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL'))
      .ref();

  final DatabaseReference chatListRef =_databaseReference.child('chatlist').child(userId);

  return chatListRef.onValue.map((event) {
    final data = event.snapshot.value;
    final List<ChatInfo> chatList = [];
    if (data is Map) {
      data.forEach((key, value) {
        final chatInfo = ChatInfo.fromJson(Map<String, dynamic>.from(value));
        chatInfo.createId = key;
        chatList.add(chatInfo);
      });
    }
    return chatList;
  });
}

Future<bool> addChatMessage(
    String currentUserId,
    ChatMessage chatMessage,
    String friendId,
    ) async {
  final DatabaseReference _databaseReference = FirebaseDatabase.instanceFor(
      app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL'))
      .ref();

  try {
    await _databaseReference.child("chat")
        .child(getRoomId(
        FirebaseAuth.instance.currentUser!.uid, friendId))
        .child("details")
        .push()
        .set(chatMessage.toJson());
    return true;
  } catch (e) {
    print(e);
    return false;
  }
}


Future<List<ChatMessage>> getChatHistory(String roomId) async{
  final DatabaseReference _databaseReference = FirebaseDatabase.instanceFor(
      app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL'))
      .ref();


  var list = <ChatMessage>[];
  var source = await _databaseReference.child('chatlist')
      .child(roomId)
      .once();
  var values = source.snapshot.value;
  ChatMessage chatMessage;

  if (values is Map) {
    values.forEach((key, value) {
      chatMessage = ChatMessage.fromJson(Map<String, dynamic>.from(value));
      chatMessage.uid = key;
      list.add(chatMessage);
    });
  }

  return list;
}
