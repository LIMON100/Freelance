package com.axon.kisa10.adapter

import android.annotation.SuppressLint
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.axon.kisa10.adapter.ListItemDialogAdapter.ListItemDialogViewHolder
import com.axon.kisa10.model.ListItemDialog
import com.axon.kisa10.R

class ListItemDialogAdapter(
    private val listItemDialogs: ArrayList<ListItemDialog>,
    private val onItemClickListener : (Int) -> Unit
) : RecyclerView.Adapter<ListItemDialogViewHolder>() {


    inner class ListItemDialogViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        val mImgCheck: ImageView = itemView.findViewById(R.id.img_check_et)
        val mTvName: TextView = itemView.findViewById(R.id.tv_content_et)
        val mTabItem: View = itemView.findViewById(R.id.tab_item)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ListItemDialogViewHolder {
        val v = LayoutInflater.from(parent.context).inflate(R.layout.item_dialog, parent, false)
        val viewHolder = ListItemDialogViewHolder(v)
        return viewHolder
    }

    override fun onBindViewHolder(holder: ListItemDialogViewHolder, position: Int) {
        val listItemDialog = listItemDialogs[position]
        holder.mTvName.text = listItemDialog.getName()
        holder.mTvName.textSize = 17f
        holder.mTabItem.setOnClickListener {
            if (holder.mImgCheck.isSelected) {
                holder.mImgCheck.isSelected = false
                holder.mTvName.isSelected = false
                holder.mImgCheck.setBackgroundResource(R.drawable.bg_check)
            } else {
                holder.mImgCheck.isSelected = true
                holder.mTvName.isSelected = true
                holder.mImgCheck.setBackgroundResource(R.drawable.bg_uncheck)
            }
            onItemClickListener(position)
            notifyDataSetChanged()
        }
        if (holder.mImgCheck.isSelected) {
            listItemDialog.setCheck(true)
        } else listItemDialog.setCheck(false)

        if (holder.mTvName.isSelected) {
            listItemDialog.setCheck(true)
        } else listItemDialog.setCheck(false)
    }

    override fun getItemCount(): Int {
        return listItemDialogs.size
    }
}

interface OnItemClickListener {
    fun onItemClick(position: Int)
}