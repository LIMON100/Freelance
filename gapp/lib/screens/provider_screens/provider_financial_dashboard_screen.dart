import 'package:flutter/material.dart';
import 'package:flutter/widgets.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:groom/utils/tools.dart';
import 'package:groom/ui/provider/home/widgets/bar_chart_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/loading_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';

import '../../firebase/provider_user_firebase.dart';
import '../../repo/setting_repo.dart';
import '../../ui/provider/home/m/transaction.dart';
import '../../utils/colors.dart';

class ProviderFinancialDashboard extends StatefulWidget {
  const ProviderFinancialDashboard({super.key});

  @override
  State<ProviderFinancialDashboard> createState() =>
      _ProviderFinancialDashboardState();
}

class _ProviderFinancialDashboardState
    extends State<ProviderFinancialDashboard> {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: HeaderTxtWidget(
          "Financial Dashboard",
          color: Colors.white,
        ),
      ),
      body: SingleChildScrollView(
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            BarChartWidget(),
            const Divider(),
            Padding(padding: const EdgeInsets.symmetric(horizontal: 10,vertical: 10),
            child: HeaderTxtWidget('Transaction'),),
            FutureBuilder(
              future: ProviderUserFirebase().getAllTransaction(auth.value.uid),
              builder: (context, snapshot) {
                if(snapshot.data==null){
                  return LoadingWidget();
                }
                if(snapshot.data!.isEmpty){
                  return Container(
                    padding: const EdgeInsets.all(15),
                    child: SubTxtWidget("No data found"),
                  );
                }
                return ListView.builder(
                  padding: EdgeInsets.zero,
                  itemBuilder: (context, index) {
                    TransactionModel data=snapshot.data![index];
                    return ListTile(
                      title: SubTxtWidget(data.paymentMode!),
                      subtitle: SubTxtWidget(Tools.changeDateFormat(data.createdDate!, globalTimeFormat),fontSize: 12,),
                      trailing: SubTxtWidget('${data.amount} USD'),
                    );
                  },
                  itemCount: snapshot.data!.length,
                  shrinkWrap: true,
                );
              },
            )
          ],
        ),
      ),
    );
  }
}
