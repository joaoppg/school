#!/bin/python
#Author: 	Martin Krajňák, xkrajn022stud.fit.vutbr.cz
#Desc:		downloading tweets, projekt for ISJ course

import re
import tweepy
import urllib3
import ssl
import certifi
import os

#setting up variables fo authentification to twitter
consumer_key="jOOriBb6TIdBKozNFUGnxY5Ar"
consumer_secret="nAoXWufN2XBM3EY1NvDgZwmGTiFpyjHHB4tNSfj0JyINnd35si"
access_token="3231957327-7Px8f3rWVyhFie6wt1YiTIEQXLmzUOLYiqIFeGd"
access_token_secret="WHOo5xXSchr9iTStL0wd9HHQPyT0nZiUYrS92JV8mUbs4"

#screen name of account 
screen_name = "BattleForceCZE"

#Authetification and setting up api 
auth = tweepy.OAuthHandler(consumer_key, consumer_secret)
auth.set_access_token(access_token, access_token_secret)
api = tweepy.API(auth)

#lists init
tweets = []
links =[]

#getting 3rd party links from tweets
def get_links(tweets):
	for tweet in tweets:
		links.extend(re.findall("(https?:\/\/t.co\/[a-zA-Z0-9]{10})",tweet.text)) 
	n=0
	for link in links:
		n += 1
		download_link(link,tweet,n)
	return

#downloading links, n link number in one tweet
def download_link(link,tweet,n):
	http = urllib3.PoolManager(
    		cert_reqs='CERT_REQUIRED', # Force certificate check.
    		ca_certs=certifi.where(),  # Path to the Certifi bundle.
    		retries=3
	)
	try:
		page = http.request('GET',link)
		path = "pages/p"+str(tweet.id)+"_"+str(n)
		check_directory("pages")
		f = open(path, "wb")
		f.write(page.data)
		f.close()
		page.release_conn()
		#print("done")
	except:
		print ("ERR while downloading a page%s"%link)
	return

#check whether directory exists, create if necessary
def check_directory(directory):
	if not os.path.exists(directory):
		os.makedirs(directory)
	return

#getting id of last tweet
def get_newest_id():
	newest_id= ""
	f = open("tweets","r")	
	lines = f.readlines()
	for line in lines:
			if "<id>" in line:
				newest_id=re.search("<id>(.*)</id>",line).group(1)
				print("match")
				print(line)
				break
	f.close()
	return newest_id
#will download tweets
def get_all_tweets(count):
	i = 0
	try:
		temp = api.user_timeline(screen_name, count=count)
	except:
		print("ERR: timeline call failed")
	tweets.extend(temp)
	while len(temp)>0:
		last = tweets[-1].id-1
		try:
			temp = api.user_timeline(screen_name, count=count,max_id=last)
		except:
			print("ERR: timeline call failed")
		i += 1
		if (len(temp) != 200):
			print("Warning, not all requested tweets was downloaded")
		print("#%d,Downloaded %d tweets"%(i,len(temp)))
		tweets.extend(temp)
	return

def update_tweets(count,since_id):
	i = 0
	temp = []
	try:
		temp = api.user_timeline(screen_name, count=count,since_id=last)
	except:
		print("ERR: timeline call error, or already all posts downloaded")
	if (len(temp) != 200):
		print("Warning, not all requested tweets was downloaded")
	print("#%d,Downloaded %d tweets"%(i,len(temp)))
	tweets.extend(temp)
	return
#saving downloaded tweets to a xml like file
def save_tweets(tweets):
	f = open("tweets","a")
	f.write("<tweets>\n")
	print("Saving tweets, downloading websites from url")
	for tweet in tweets:
		#writing tweets to a file
		f.write("<tweet>\n")
		f.write("<author>"+str(tweet.author.name)+"</author>\n")
		f.write("<datetime>"+str(tweet.created_at)+"</datetime>\n")
		f.write("<id>"+str(tweet.id)+"</id>\n")
		f.write("<text>"+str(tweet.text.encode('utf-8'))+"</text>\n")
		f.write("</tweet>\n")
	f.write("</tweets>\n")
	f.close()
	return

#checking if log file exists
print('Started')
try:
	if os.stat("tweets").st_size > 0:
		#get id of last downloaded tweet
		newest_id = get_newest_id();
		if(newest_id != ""):	
			print("Downloading new posts since id %s:"%newest_id)
			update_tweets(200,since_id=newest_id)
		else:
			print("newest id not found, file might be corrupted")
	else:
		#empty file
		get_all_tweets(200)
except OSError:
		#File not found, creating new via open
		get_all_tweets(200)
		pass
if (tweets != []):
	save_tweets(tweets)
	print("Successfully collected %d tweets"%len(tweets))
	print("Trying to get all the links")
	get_links(tweets)
	