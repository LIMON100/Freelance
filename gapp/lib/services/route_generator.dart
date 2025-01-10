import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/ui/customer/customer_profile/customer_profile_page.dart';
import '../ui/customer/customer_dashboard/all_provider_list.dart';
import '../ui/customer/customer_dashboard/all_service_list.dart';
import '../ui/customer/report/provider_report_page.dart';
import '../ui/customer/report/report_page.dart';
import '../ui/customer/service_request/confirm_service_request_page.dart';
import '../ui/invite/invite_page.dart';
import '../ui/membership/membership_active_plan_page.dart';
import '../ui/provider/addService/provider_services_list.dart';
import '../screens/term_services_screen.dart';
import '../ui/about/about_page.dart';
import '../ui/customer/booking/customer_booking_screen.dart';
import '../ui/customer/payment/payment_page.dart';
import '../ui/customer/search/search_maps_screen.dart';
import '../ui/customer/search/search_page.dart';
import '../ui/login/login_success_page.dart';
import '../ui/provider/booking/provider_booking_screen.dart';
import '../ui/provider/calendar/calendar_page.dart';
import '../ui/provider/category/category_page.dart';
import '../ui/provider/home/provider_home_screen.dart';
import '../ui/customer/customer_dashboard/customer_home_screen.dart';
import '../ui/customer/search/customer_service_search_screen.dart';
import '../ui/customer/customer_dashboard/customer_dashboard_page.dart';
import '../ui/customer/customer_profile/customer_edit_profile_screen.dart';
import '../ui/login/login_page.dart';
import '../ui/login/otp_page.dart';
import '../ui/onboard/onboard_page.dart';
import '../ui/provider/reviews/review_list.dart';
import '../ui/provider/schedule/schedule_page.dart';
import '../ui/provider/skills/skills_page.dart';
import '../ui/signup/signup_page.dart';
import '../ui/splash/splash_page.dart';
import '../widgets/header_txt_widget.dart';

appRoutes() => [
      GetPage(
        name: '/splash',
        page: () => SplashPage(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/onboard',
        page: () => OnboardPage(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/login',
        page: () => LoginPage(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/otp',
        page: () => OtpPage(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/signup',
        page: () => SignupPage(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/customer_dashboard',
        page: () => CustomerDashboardPage(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/edit_profile',
        page: () => EditProfileScreen(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/search_map_screen',
        page: () => const SearchMapScreen(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/payment_screen',
        page: () => PaymentPage(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/term_condition',
        page: () => const CustomerTerms(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/login_success',
        page: () => LoginSuccessPage(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/search',
        page: () => SearchPage(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/about',
        page: () => AboutPage(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/provider_service_list',
        page: () => ServiceProviderList(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/review_list',
        page: () => ReviewList(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/schedule',
        page: () => SchedulePage(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/skills',
        page: () => SkillsPage(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/category',
        page: () => CategoryPage(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/invite',
        page: () => InvitePage(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/all_provider_list',
        page: () => const AllProviderList(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/confirm_service_request_list',
        page: () => ConfirmServiceRequestPage(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/add_provider_report',
        page: () => ProviderReportPage(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/report',
        page: () => ReportPage(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
      GetPage(
        name: '/all_service_list',
        page: () => AllServiceList(),
        transitionDuration: const Duration(milliseconds: 500),
      ),GetPage(
        name: '/membership_active_plan',
        page: () =>  MembershipActivePlanPage(),
        transitionDuration: const Duration(milliseconds: 500),
      ),
    ];

Route? onGenerateRoute(RouteSettings settings) {
  print("name ${settings.name}");
  if (settings.name == 'home') {
    return GetPageRoute(
      settings: settings,
      page: () => const CustomerHomeScreen(),
    );
  }
  if (settings.name == 'profile') {
    return GetPageRoute(
      settings: settings,
      page: () => CustomerProfilePage(),
    );
  }
  if (settings.name == 'booking') {
    return GetPageRoute(
      settings: settings,
      page: () => const CustomerBookingScreen(),
    );
  }
  if (settings.name == 'search') {
    return GetPageRoute(
      settings: settings,
      page: () => SearchScreen(),
    );
  }
  if (settings.name == 'provider_home') {
    return GetPageRoute(
      settings: settings,
      page: () => const ProviderHomeScreen(),
    );
  }
  if (settings.name == 'provider_booking') {
    return GetPageRoute(
      settings: settings,
      page: () => const BookingScreen(),
    );
  }
  if (settings.name == 'calendar') {
    return GetPageRoute(
      settings: settings,
      page: () => CalendarPage(),
    );
  }

  return GetPageRoute(
    settings: settings,
    page: () => Container(
      alignment: AlignmentDirectional.center,
      child: HeaderTxtWidget("Page On Development"),
    ),
  );
}

class MyMiddelware extends GetMiddleware {
  @override
  GetPage? onPageCalled(GetPage? page) {
    print(page?.name);
    return super.onPageCalled(page);
  }
}
