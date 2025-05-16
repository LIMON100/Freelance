package com.axon.kisa10.util;

import java.util.Vector;

/**
 * Created by Hussain on 14/09/24.
 */
public class LogUtil {

    private static final char[] HEX_ARRAY = "0123456789ABCDEF".toCharArray();
    public static String bytesToHex(byte[] bytes) {
        Vector<String> hex_str = new Vector<String>();
        Vector<String> ascii_str = new Vector<String>();
        String hex="";
        String ascii="";
        for (int j = 0; j < bytes.length; j++) {
            int v = bytes[j] & 0xFF;
            hex += HEX_ARRAY[v >>> 4];
            hex += HEX_ARRAY[v & 0x0F];
            hex += " ";
            if(v>=32 && v<=126) {
                ascii += (char)v;
            } else {
                ascii += ".";
            }
            if(((j+1)%16)==0) {
                hex_str.add(hex);
                hex = "";
                ascii_str.add(ascii);
                ascii = "";
            }
        }
        if(hex.length()>0) {
            hex_str.add(hex);
            ascii_str.add(ascii);
        }

        hex = "";
        for (int i = 0; i < hex_str.size(); i++) {
            String str = hex_str.elementAt(i);
            hex += str;
            hex += "  ";
            if(str.length()<48) {
                for(int j=0;j<48-str.length();j++) {
                    hex += " ";
                }
            }
            hex += ascii_str.elementAt(i);
            hex += "\n";
        }

        return hex;
    }

    public static String convertByteToString(byte[] data, int count)
    {
        String ret = "";
        if(data[0]!='R') return ret;
        if(count > data.length) {
            count = data.length;
        }
        for(int i=0;i<count;i++) {
            final int c = data[i];
            if(Character.isDefined(c)) {
                ret += (char)c;
            } else {
                break;
            }
        }
        return ret;
    }
}
