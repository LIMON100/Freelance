package com.axon.kisa10.util;

import java.security.InvalidParameterException;

/**
 * @author Toan Vo <toanvo@adasone.com>
 * @brief Implement CRC32 algorithm
 *        CRC_CR: 0x0000 0000; POLYSIZE is 32, No REV_IN and No REV_OUT
 *        CRC_INIT: 0XFFFF FFFF
 *        CRC_POLY: 0X04D11 CDB7
 */

public class AxonCRC32 {
    private final int CRC32_LUT_SIZE = 256;
    private int[] m_lut;
    private int m_value = 0xffffffff;

    public AxonCRC32()
    {
        initLUT();
    }

    private void initLUT()
    {
        m_lut = new int[CRC32_LUT_SIZE];
        m_value = 0xffffffff;
        for(int i=0; i < CRC32_LUT_SIZE; ++i) {
            int k = 0;
            int j = (i << 24) | 0x800000;
            while(j != 0x80000000) {
                int x = (((k ^ j) & 0x80000000) != 0) ? 0x04c11db7 : 0;
                k = (k << 1) ^ x;
                j = j << 1;
            }
            m_lut[i] = k;
        }
    }

    public void update(byte[] data, int len) throws InvalidParameterException
    {
        int crc = m_value;
        if(len == 0) throw new InvalidParameterException("data length must not be zero");
        for(int i=0; i < len; i += 4) {
            final int k = ((i+3)<len)?(i+3):(len-1);
            for(int j = k; j >= i; --j) {
                crc = (crc << 8) ^ m_lut[((crc >> 24) ^ data[j]) & 0xff];
            }
        }
        m_value = crc;
    }

    public void reset()
    {
        m_value = 0xffffffff;
    }

    public int getValue()
    {
        return m_value;
    }

}
