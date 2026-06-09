package com.example.myapplication.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.example.myapplication.R
import com.example.myapplication.model.MomentPost

class MomentsAdapter(
    private val posts: List<MomentPost>
) : RecyclerView.Adapter<MomentsAdapter.ViewHolder>() {

    class ViewHolder(view: View) : RecyclerView.ViewHolder(view) {
        val author: TextView = view.findViewById(R.id.tv_author)
        val content: TextView = view.findViewById(R.id.tv_content)
        val time: TextView = view.findViewById(R.id.tv_time)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_moment, parent, false)
        return ViewHolder(view)
    }

    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        val post = posts[position]
        holder.author.text = post.author
        holder.content.text = post.content
        holder.time.text = post.time
    }

    override fun getItemCount(): Int = posts.size

    fun authorAt(position: Int): String =
        posts.getOrNull(position)?.author ?: ""
}
