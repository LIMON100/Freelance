import 'package:flutter/services.dart' show rootBundle;
import 'dart:convert';

class GlobalConfiguration {
  static GlobalConfiguration _singleton = GlobalConfiguration._internal();

  factory GlobalConfiguration() {
    return _singleton;
  }

  GlobalConfiguration._internal();

  Map<String, dynamic> appConfig = Map<String, dynamic>();

  GlobalConfiguration loadFromMap(Map<String, dynamic> map) {
    appConfig.addAll(map);
    return _singleton;
  }

  Future<GlobalConfiguration> loadFromAsset(String name) async {
    if (!name.endsWith(".json")) {
      name = "$name.json";
    }
    String content = await rootBundle.loadString("assets/cnf/$name");

    Map<String, dynamic> configAsMap = json.decode(content);
    appConfig.addAll(configAsMap);
    return _singleton;
  }


  Future<GlobalConfiguration> loadFromPath(String path) async {
    String content = await rootBundle.loadString(path);
    Map<String, dynamic> configAsMap = json.decode(content);
    appConfig.addAll(configAsMap);
    return _singleton;
  }

  Future<GlobalConfiguration> loadFromPathIntoKey(
      String path, String key) async {
    String content = await rootBundle.loadString(path);
    Map<String, dynamic> configAsMap = json.decode(content);
    appConfig.putIfAbsent(key, () => configAsMap);
    return _singleton;
  }




  dynamic get(String key) => appConfig[key];

  T getValue<T>(String key) => appConfig[key] as T;




  void clear() => appConfig.clear();

  void updateValue(String key, dynamic value) {
    if (appConfig[key] != null &&
        value.runtimeType != appConfig[key].runtimeType) {
      throw ("The persistent type of ${appConfig[key].runtimeType} does not match the given type ${value.runtimeType}");
    }
    appConfig.update(key, (dynamic) => value);
  }

  void addValue(String key, dynamic value) =>
      appConfig.putIfAbsent(key, () => value);

  add(Map<String, dynamic> map) => appConfig.addAll(map);

}