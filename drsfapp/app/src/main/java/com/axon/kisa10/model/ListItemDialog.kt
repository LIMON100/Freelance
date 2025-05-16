package com.axon.kisa10.model

class ListItemDialog(private var mImgCheck: Int, private var mName: String) {
    private var isCheck: Boolean = false

    fun getImgCheck(): Int {
        return mImgCheck
    }

    fun setImgCheck(mImgCheck: Int) {
        this.mImgCheck = mImgCheck
    }

    fun getName(): String {
        return mName
    }

    fun setName(mName: String) {
        this.mName = mName
    }

    fun isCheck(): Boolean {
        return isCheck
    }

    fun setCheck(check: Boolean) {
        isCheck = check
    }
}
