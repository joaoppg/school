#!/bin/python

#Author: 	Martin Krajňák, xkrajn022stud.fit.vutbr.cz
#Desc:		downloading forum posts, projekt for ISJ course
import urllib3
import requests
import certifi
import re
import os, sys
from bs4 import BeautifulSoup 
from bs4 import SoupStrainer 

FIRST_PAGE = "http://forum.battleforce.cz/"	
MAIN = 'main'
PREFIX = '.html'
NAV = '&start='

#check whether directory exists, create if necessary
def check_directory(directory):
	if not os.path.isdir(directory):
		os.makedirs(directory)
	return

#saves first page if forum as main.html
def download_page(link,d,file_name):
	http = urllib3.PoolManager()
	next_class = SoupStrainer(class_='pagination pagination-sm hidden-xs')
	try:
		page = http.request('GET',link)
		try:
			if(d == None):
				with open('main.html', "wb") as f:
					f.write(page.data)
				f.close()
			else:
				check_directory(d)
				f = open("%s/%s.html"%(d,file_name), "ab")
				f.write(page.data)
				while(True):
					data = BeautifulSoup(page.data,"html.parser", parse_only=next_class).prettify()
					soup = BeautifulSoup(data)
					for href in soup.findAll('a',href=True):
						if 'Další' in href.getText():
							try:
								print(c)
								page = http.request('GET',FIRST_PAGE+href['href'])			
								f.write(page.data)
								break 
							except:
								print('ERR while downloading next page')
					else:
						break
				f.close()
		except:
			print("ERR Cannot write to a file")
		page.release_conn()
		return True
	except:
		print ("ERR while downloading a page %s"%link)
		return False

#get new post count + topics count from main.page
def get_counts():
	only_class = SoupStrainer(class_="stats-wrapper")
	only_br = SoupStrainer("br")
	data = BeautifulSoup(open('main.html'),"html.parser", parse_only=only_class).prettify()
	posts=""
	topics=""
	post_count = 0
	topics_count = 0
	for line in data.splitlines():
		if "Příspěvky" in line:
			posts += line
		elif "Témata" in line:
			topics += line
	post_numbers = map(int, re.findall('\d+', posts))
	topics_numbers = map(int, re.findall('\d+', topics))
	for num in post_numbers:
		post_count += num
	for num in topics_numbers:
		topics_count += num
	return (post_count,topics_count)

#will open the page as Soup(posts) and get the post class
def parse_post(name):
	#posts_class = SoupStrainer(class_='postbody')
	#posts = BeautifulSoup(open(name),'html.parser',parse_only=posts_class).prettify()
	parsed_posts = BeautifulSoup(open(name))
	with open(name,'w') as f:
		f.write(" ")
	f.close()
	for post in parsed_posts.find_all('div',class_='postbody'):	
		with open(name,'a') as f:
			f.write('<post>\n'+'<time>') 
			for s in post.span.stripped_strings:
				f.write(s) 		#postnumber
			f.write('</time>\n<postnumber>')
			for s in post.a.stripped_strings:
				f.write(s)
			f.write('</postnumber>\n')
			f.write('<content>\n')			#date
			for s in post.div.stripped_strings:	
				f.write(s)	#
			f.write('</content>\n'+'</post>\n')
	f.close()
	return

#check if thread has next page
def find_next(soup):
	next_class = SoupStrainer(class_='pagination pagination-sm hidden-xs')
	for href in soup.find_all('a',href=True):
		if 'Další' in href.getText():
			return True
	return False

#will get posts from html file
def get_posts(name):
	topic_class = SoupStrainer(class_="topictitle")
	topic_links = []
	subforum_topics = BeautifulSoup(open("%s/main.html"%name),"html.parser", parse_only=topic_class).prettify()
	subforum = BeautifulSoup(subforum_topics)
	for link in subforum.findAll('a', href=True):
		topic_links.append(FIRST_PAGE+link['href'])
	for topic_name,link in zip(subforum.stripped_strings, topic_links):
		topic_name = topic_name.replace('/','\\')
		download_page(link,name,topic_name)
		parse_post(name+'/'+topic_name+PREFIX)
	return

#will find subforums hrefs, in page, + will download posts in normal threads
def get_subforum(name):
	forum_class = SoupStrainer(class_="forumtitle")
	more = BeautifulSoup(open("%s/main.html"%name),"html.parser", parse_only=forum_class).prettify()
	subforum_links = []
	get_more = BeautifulSoup(more)
	for link in get_more.findAll('a', href=True):
		subforum_links.append(FIRST_PAGE+link['href'])
	if(subforum_links != []):	
		for forum_name,link in zip(get_more.stripped_strings, subforum_links):
			new_name=name+"/"+forum_name
			if (download_page(link,new_name,MAIN)):
				get_subforum(new_name)
	get_posts(name)
	return

#main get main page, and iterating through topics
def main():
	print('started')
	forum_class = SoupStrainer(class_="forumtitle")
	download_page(FIRST_PAGE,None,MAIN)
	forum_data = BeautifulSoup(open('main.html'),"html.parser",parse_only=forum_class).prettify()
	main_forum = BeautifulSoup(forum_data)
	forum_links = []
	for link in main_forum.findAll('a', href=True):
		forum_links.append(FIRST_PAGE+link['href'])
	for forum_name,link in zip(main_forum.stripped_strings, forum_links):
		download_page(link,forum_name,MAIN)
		get_subforum(forum_name)
	print ("All done")
	return
main()