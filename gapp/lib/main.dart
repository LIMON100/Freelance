import 'package:country_code_picker/country_code_picker.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:get/get.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:groom/services/route_generator.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/utils/tools.dart';
import 'constant/common_constant.dart';
import 'constant/enums.dart';
import 'generated/l10n.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();

  CommonConstant.initializeApp(Mode.DEV);

  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return GetMaterialApp(
      title: 'Groom',
      navigatorKey: Tools.navigatorKey,
      debugShowCheckedModeBanner: false,
      themeMode: ThemeMode.light,
      getPages: appRoutes(),
      initialRoute: '/splash',
      theme: ThemeData(
          primaryColor: primaryColorCode,
          secondaryHeaderColor: primaryColorCode,
          primaryColorLight: primaryColorCode,
          appBarTheme: AppBarTheme(
              backgroundColor: primaryColorCode,
              iconTheme: const IconThemeData(color: Colors.white),
              systemOverlayStyle: SystemUiOverlayStyle(
                statusBarColor: primaryColorCode,
              )),
          scaffoldBackgroundColor: scaffoldBackgroundColor,
          useMaterial3: true,
          textTheme: TextTheme(
            bodyLarge: GoogleFonts.fahkwang(),
          ),
          brightness: Brightness.light),
      locale: Get.locale,
      localizationsDelegates: const [
        // S.delegate,
        GlobalMaterialLocalizations.delegate,
        GlobalCupertinoLocalizations.delegate,
        GlobalWidgetsLocalizations.delegate,
        CountryLocalizations.delegate,
      ],
      supportedLocales: S.delegate.supportedLocales,
    );
  }
}
