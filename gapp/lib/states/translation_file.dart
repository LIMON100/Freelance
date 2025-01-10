import 'package:get/get.dart';

class Translation extends Translations{
  @override
  // TODO: implement keys
  Map<String, Map<String, String>> get keys => {
    'en_US':{

      "Menu": "Menu",
      "User Profile":"User Profile",
      "Messages": "Messages",
      "Notifications": "Notifications",
      "My Hiring": "My Hiring",
      "My Jobs": "My Jobs",
      "Create Job": "Create Job",
      "Find Jobs": "Find Jobs",
      "Log Out": "Log Out",
      "Company Name":"Company Name"



    },
    'es_ES':{
      "Menu": "Menú",
      "User Profile":"perfil del usuario",
      "Messages": "Mensajes",
      "Notifications": "Notificaciones",
      "My Hiring": "Mi contratacion",
      "My Jobs": "Mis trabajos",
      "Create Job": "Crear trabajo",
      "Find Jobs": "Encontrar trabajos",
      "Log Out": "Cerrar sesión",
      "Company Name" : "Nombre de empresa"
    }
  };

}