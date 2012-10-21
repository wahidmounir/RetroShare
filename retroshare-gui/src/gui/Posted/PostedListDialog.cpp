/*
 * Retroshare Posted List
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include "PostedListDialog.h"

#include "gui/gxs/PostedGroupDialog.h"

#include <retroshare/rsposted.h>

#include <iostream>
#include <sstream>
#include <algorithm>

#include <QTimer>
#include <QMenu>
#include <QMessageBox>

/****************************************************************
 */

//#define DEBUG_FORUMS

/* Images for context menu icons */
#define IMAGE_MESSAGE        ":/images/folder-draft.png"
#define IMAGE_MESSAGEREPLY   ":/images/mail_reply.png"
#define IMAGE_MESSAGEREMOVE  ":/images/mail_delete.png"
#define IMAGE_DOWNLOAD       ":/images/start.png"
#define IMAGE_DOWNLOADALL    ":/images/startall.png"

/* Images for TreeWidget */
#define IMAGE_FOLDER         ":/images/folder16.png"
#define IMAGE_FOLDERGREEN    ":/images/folder_green.png"
#define IMAGE_FOLDERRED      ":/images/folder_red.png"
#define IMAGE_FOLDERYELLOW   ":/images/folder_yellow.png"
#define IMAGE_FORUM          ":/images/konversation16.png"
#define IMAGE_SUBSCRIBE      ":/images/edit_add24.png"
#define IMAGE_UNSUBSCRIBE    ":/images/cancel.png"
#define IMAGE_INFO           ":/images/info16.png"
#define IMAGE_NEWFORUM       ":/images/new_forum16.png"
#define IMAGE_FORUMAUTHD     ":/images/konv_message2.png"
#define IMAGE_COPYLINK       ":/images/copyrslink.png"

/** Constructor */
PostedListDialog::PostedListDialog(QWidget *parent)
: RsAutoUpdatePage(1000,parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

	/* Setup Queue */
        mPostedQueue = new TokenQueue(rsPosted->getTokenService(), this);

    connect( ui.groupTreeWidget, SIGNAL( treeCustomContextMenuRequested( QPoint ) ), this, SLOT( groupListCustomPopupMenu( QPoint ) ) );

    connect( ui.groupTreeWidget, SIGNAL( treeCurrentItemChanged(QString) ), this, SLOT( changedTopic(QString) ) );

    /* Initialize group tree */
    //ui.groupTreeWidget->initDisplayMenu(ui.displayButton);

    /* create forum tree */
    yourTopics = ui.groupTreeWidget->addCategoryItem(tr("Your Topics"), QIcon(IMAGE_FOLDER), true);
    subscribedTopics = ui.groupTreeWidget->addCategoryItem(tr("Subscribed Topics"), QIcon(IMAGE_FOLDERRED), true);
    popularTopics = ui.groupTreeWidget->addCategoryItem(tr("Popular Topics"), QIcon(IMAGE_FOLDERGREEN), false);
    otherTopics = ui.groupTreeWidget->addCategoryItem(tr("Other Topics"), QIcon(IMAGE_FOLDERYELLOW), false);

    ui.hotSortButton->setChecked(true);
    mSortButton = ui.hotSortButton;

    connect( ui.newTopicButton, SIGNAL( clicked() ), this, SLOT( newGroup() ) );

    connect( ui.hotSortButton, SIGNAL( released() ), this, SLOT( sortButtonPressed() ) );
    connect( ui.newSortButton, SIGNAL( released() ), this, SLOT( sortButtonPressed() ) );
    connect( ui.topSortButton, SIGNAL( released() ), this, SLOT( sortButtonPressed() ) );

    connect( ui.sortGroup, SIGNAL( buttonClicked( QAbstractButton * ) ), this, SLOT( sortButtonClicked( QAbstractButton * ) ) );
    connect( ui.periodComboBox, SIGNAL( currentIndexChanged ( int index ) ), this, SLOT( periodChanged ( int ) ) );

    /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}


void PostedListDialog::groupListCustomPopupMenu( QPoint /*point*/ )
{
    QMenu contextMnu( this );

    QAction *action = contextMnu.addAction(QIcon(IMAGE_SUBSCRIBE), tr("Subscribe to Forum"), this, SLOT(subscribeToForum()));
    //action->setDisabled (mCurrTopicId.empty() || IS_FORUM_SUBSCRIBED(subscribeFlags));

    action = contextMnu.addAction(QIcon(IMAGE_UNSUBSCRIBE), tr("Unsubscribe to Forum"), this, SLOT(unsubscribeToForum()));
    //action->setEnabled (!mCurrTopicId.empty() && IS_FORUM_SUBSCRIBED(subscribeFlags));

    contextMnu.addSeparator();

    contextMnu.addAction(QIcon(IMAGE_NEWFORUM), tr("New Forum"), this, SLOT(newforum()));

    action = contextMnu.addAction(QIcon(IMAGE_INFO), tr("Show Forum Details"), this, SLOT(showForumDetails()));
    action->setEnabled (!mCurrTopicId.empty ());

    action = contextMnu.addAction(QIcon(":/images/settings16.png"), tr("Edit Forum Details"), this, SLOT(editForumDetails()));
    //action->setEnabled (!mCurrTopicId.empty () && IS_FORUM_ADMIN(subscribeFlags));

    QAction *shareKeyAct = new QAction(QIcon(":/images/gpgp_key_generate.png"), tr("Share Forum"), &contextMnu);
    connect( shareKeyAct, SIGNAL( triggered() ), this, SLOT( shareKey() ) );
    //shareKeyAct->setEnabled(!mCurrTopicId.empty() && IS_FORUM_ADMIN(subscribeFlags));
    contextMnu.addAction( shareKeyAct);

    QAction *restoreKeysAct = new QAction(QIcon(":/images/settings16.png"), tr("Restore Publish Rights for Forum" ), &contextMnu);
    connect( restoreKeysAct , SIGNAL( triggered() ), this, SLOT( restoreForumKeys() ) );
    //restoreKeysAct->setEnabled(!mCurrTopicId.empty() && !IS_FORUM_ADMIN(subscribeFlags));
    contextMnu.addAction( restoreKeysAct);

    action = contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyForumLink()));
    action->setEnabled(!mCurrTopicId.empty());

    contextMnu.addSeparator();

    action = contextMnu.addAction(QIcon(":/images/message-mail-read.png"), tr("Mark all as read"), this, SLOT(markMsgAsReadAll()));
    //action->setEnabled (!mCurrTopicId.empty () && IS_FORUM_SUBSCRIBED(subscribeFlags));

    action = contextMnu.addAction(QIcon(":/images/message-mail.png"), tr("Mark all as unread"), this, SLOT(markMsgAsUnreadAll()));
    //action->setEnabled (!mCurrTopicId.empty () && IS_FORUM_SUBSCRIBED(subscribeFlags));

    contextMnu.exec(QCursor::pos());
}

void PostedListDialog::updateDisplay()
{
    std::list<std::string> groupIds;
    std::list<std::string>::iterator it;
    if (!rsPosted)
        return;

	// TODO groupsChanged... HACK XXX.
#if 0
    if ((rsPosted->groupsChanged(groupIds)) || (rsPosted->updated()))
    {
        /* update Forums List */
        insertGroups();

        it = std::find(groupIds.begin(), groupIds.end(), mCurrTopicId);
        if (it != groupIds.end())
        {
            /* update threads as well */
            insertThreads();
        }
    }
#endif

    if (rsPosted->updated())
    {
        /* update Forums List */
        insertGroups();
        insertThreads();
    }

}

void PostedListDialog::requestComments(std::string threadId)
{
	/* call a signal */
	std::cerr << "PostedListDialog::requestComments(" << threadId << ")";
	std::cerr << std::endl;

	loadComments(threadId);

}


void PostedListDialog::changedTopic(const QString &id)
{
    mCurrTopicId = id.toStdString();
    insertThreads();
}

void PostedListDialog::sortButtonPressed()
{
	std::cerr << "PostedListDialog::sortButtonPressed()";
	std::cerr << std::endl;

	QAbstractButton *pressed = NULL;
	if (ui.hotSortButton->isChecked()) {
		std::cerr << "PostedListDialog::sortButtonPressed() Hot";
		std::cerr << std::endl;
		pressed = ui.hotSortButton;
	} else if (ui.newSortButton->isChecked()) {
		std::cerr << "PostedListDialog::sortButtonPressed() New";
		std::cerr << std::endl;
		pressed = ui.newSortButton;
	} else if (ui.topSortButton->isChecked()) {
		std::cerr << "PostedListDialog::sortButtonPressed() Top";
		std::cerr << std::endl;
		pressed = ui.topSortButton;
	}

	if ((pressed) && (pressed != mSortButton))
	{
		mSortButton = pressed;
		sortButtonClicked( mSortButton );
		insertThreads();
	}
}

void PostedListDialog::sortButtonClicked( QAbstractButton *button )
{
	std::cerr << "PostedListDialog::sortButtonClicked( From Button Group! )";
	std::cerr << std::endl;

        uint32_t sortMode = RSPOSTED_VIEWMODE_HOT;

	if (button == ui.hotSortButton) {
                sortMode = RSPOSTED_VIEWMODE_HOT;
	} else if (button == ui.newSortButton) {
                sortMode = RSPOSTED_VIEWMODE_LATEST;
	} else if (button == ui.topSortButton) {
                sortMode = RSPOSTED_VIEWMODE_TOP;
	}
}


void PostedListDialog::periodChanged( int index )
{
        uint32_t periodMode = RSPOSTED_PERIOD_HOUR;
	switch (index)
	{
		case 0:
                        periodMode = RSPOSTED_PERIOD_HOUR;
			break;

		case 1:
                        periodMode = RSPOSTED_PERIOD_DAY;
			break;

		default:
		case 2:
                        periodMode = RSPOSTED_PERIOD_WEEK;
			break;

		case 3:
                        periodMode = RSPOSTED_PERIOD_MONTH;
			break;

		case 4:
                        periodMode = RSPOSTED_PERIOD_YEAR;
			break;
	}
}



/*********************** **** **** **** ***********************/
/** New / Edit Groups          ********************************/
/*********************** **** **** **** ***********************/

void PostedListDialog::newGroup()
{
	PostedGroupDialog cf (this);
	cf.newGroup();
	
	cf.exec ();
}
	
void PostedListDialog::showGroupDetails()
{
	if (mCurrTopicId.empty()) 
	{
		return;
	}
	
	PostedGroupDialog cf (this);
	cf.existingGroup(mCurrTopicId,  GXS_GROUP_DIALOG_SHOW_MODE);
	
	cf.exec ();
}
	
void PostedListDialog::editGroupDetails()
{
	if (mCurrTopicId.empty()) 
	{
		return;
	}
	
	PostedGroupDialog cf (this);
	cf.existingGroup(mCurrTopicId,  GXS_GROUP_DIALOG_EDIT_MODE);
	
	cf.exec ();
}


/*********************** **** **** **** ***********************/
/** Request / Response of Data ********************************/
/*********************** **** **** **** ***********************/

#define 	POSTEDDIALOG_LISTING			1
#define 	POSTEDDIALOG_CURRENTFORUM		2
#define 	POSTEDDIALOG_INSERTTHREADS		3
#define 	POSTEDDIALOG_INSERTCHILD		4
#define 	POSTEDDIALOG_INSERT_POST		5
#define 	POSTEDDIALOG_REPLY_MESSAGE		6


void PostedListDialog::insertGroups()
{
	requestGroupSummary();
}

void PostedListDialog::requestGroupSummary()
{
        std::cerr << "PostedListDialog::requestGroupSummary()";
        std::cerr << std::endl;

        std::list<std::string> ids;
        RsTokReqOptions opts;
	uint32_t token;
        mPostedQueue->requestGroupInfo(token,  RS_TOKREQ_ANSTYPE_SUMMARY, opts, ids, POSTEDDIALOG_LISTING);
}

void PostedListDialog::loadGroupSummary(const uint32_t &token)
{
        std::cerr << "PostedListDialog::loadGroupSummary()";
        std::cerr << std::endl;

        std::list<RsGroupMetaData> groupInfo;
        rsPosted->getGroupSummary(token, groupInfo);

        if (groupInfo.size() > 0)
        {
		insertGroupData(groupInfo);
        }
        else
        {
                std::cerr << "PostedListDialog::loadGroupSummary() ERROR No Groups...";
                std::cerr << std::endl;
        }
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/



void PostedListDialog::requestGroupSummary_CurrentForum(const std::string &forumId)
{
        RsTokReqOptions opts;
	
	std::list<std::string> grpIds;
	grpIds.push_back(forumId);

        std::cerr << "PostedListDialog::requestGroupSummary_CurrentForum(" << forumId << ")";
        std::cerr << std::endl;

	uint32_t token;	
	mPostedQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, grpIds, POSTEDDIALOG_CURRENTFORUM);
}

void PostedListDialog::loadGroupSummary_CurrentForum(const uint32_t &token)
{
        std::cerr << "PostedListDialog::loadGroupSummary_CurrentForum()";
        std::cerr << std::endl;

        std::list<RsGroupMetaData> groupInfo;
        rsPosted->getGroupSummary(token, groupInfo);

        if (groupInfo.size() == 1)
        {
		RsGroupMetaData fi = groupInfo.front();
		//insertForumThreads(fi);
        }
        else
        {
                std::cerr << "PostedListDialog::loadGroupSummary_CurrentForum() ERROR Invalid Number of Groups...";
                std::cerr << std::endl;
        }
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/


void PostedListDialog::insertThreads()
{
	loadCurrentForumThreads(mCurrTopicId);
}

void PostedListDialog::loadCurrentForumThreads(const std::string &forumId)
{

        std::cerr << "PostedListDialog::loadCurrentForumThreads(" << forumId << ")";
        std::cerr << std::endl;

	if (forumId.empty())
	{
        	std::cerr << "PostedListDialog::loadCurrentForumThreads() Empty GroupId .. ignoring Req";
        	std::cerr << std::endl;
		return;
	}

	/* if already active -> kill current loading */
	if (mThreadLoading)
	{
		/* Cleanup */
		std::cerr << "Already Loading -> must Clean ... TODO, retry in a moment";
		return;
	}

	clearPosts();

	/* initiate loading */
        std::cerr << "PostedListDialog::loadCurrentForumThreads() Initiating Loading";
        std::cerr << std::endl;

	mThreadLoading = true;

	requestGroupThreadData_InsertThreads(forumId);
}



void PostedListDialog::requestGroupThreadData_InsertThreads(const std::string &groupId)
{
        RsTokReqOptions opts;

	opts.mOptions = RS_TOKREQOPT_MSG_THREAD | RS_TOKREQOPT_MSG_LATEST;
	
	std::list<std::string> grpIds;
	grpIds.push_back(groupId);

        std::cerr << "PostedListDialog::requestGroupThreadData_InsertThreads(" << groupId << ")";
        std::cerr << std::endl;

	uint32_t token;	
	//mPostedQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, grpIds, POSTEDDIALOG_INSERTTHREADS);

	// Do specific Posted Request....
        if (rsPosted->requestRanking(token, groupId))
	{
		// get the Queue to handle response.
        	mPostedQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_DATA, POSTEDDIALOG_INSERTTHREADS);
	}
}


void PostedListDialog::loadGroupThreadData_InsertThreads(const uint32_t &token)
{
	std::cerr << "PostedListDialog::loadGroupThreadData_InsertThreads()";
	std::cerr << std::endl;
	
	bool moreData = true;
	while(moreData)
	{
                RsPostedPost post;
		// Old Format.
                //if ()

                if (/*rsPosted->getPost(token, post)*/false)
		{
			std::cerr << "PostedListDialog::loadGroupThreadData_InsertThreads() MsgId: " << post.mMeta.mMsgId;
			std::cerr << std::endl;
		
			loadPost(post);
		}
		else
		{
			moreData = false;
		}
	}

	mThreadLoading = false;

}

void PostedListDialog::loadPost(const RsPostedPost &post)
{
        PostedItem *item = new PostedItem(this, post);
        QLayout *alayout = ui.scrollAreaWidgetContents->layout();
        alayout->addWidget(item);	
}


void PostedListDialog::clearPosts()
{
        std::cerr << "PostedListDialog::clearPosts()" << std::endl;

        std::list<PostedItem *> postedItems;
        std::list<PostedItem *>::iterator pit;

        QLayout *alayout = ui.scrollAreaWidgetContents->layout();
        int count = alayout->count();
        for(int i = 0; i < count; i++)
        {
                QLayoutItem *litem = alayout->itemAt(i);
                if (!litem)
                {
                        std::cerr << "PostedListDialog::clearPosts() missing litem";
                        std::cerr << std::endl;
                        continue;
                }

                PostedItem *item = dynamic_cast<PostedItem *>(litem->widget());
                if (item)
                {
                        std::cerr << "PostedListDialog::clearPosts() item: " << item;
                        std::cerr << std::endl;

                        postedItems.push_back(item);
                }
                else
                {
                        std::cerr << "PostedListDialog::clearPosts() Found Child, which is not a PostedItem???";
                        std::cerr << std::endl;
                }
        }

        for(pit = postedItems.begin(); pit != postedItems.end(); pit++)
        {
                PostedItem *item = *pit;
                alayout->removeWidget(item);
                delete item;
        }
}



/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
	
void PostedListDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "PostedListDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
				
	if (queue == mPostedQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
			case POSTEDDIALOG_LISTING:
				loadGroupSummary(req.mToken);
				break;

			case POSTEDDIALOG_CURRENTFORUM:
				loadGroupSummary_CurrentForum(req.mToken);
				break;

			case POSTEDDIALOG_INSERTTHREADS:
				loadGroupThreadData_InsertThreads(req.mToken);
				break;

			default:
				std::cerr << "PostedListDialog::loadRequest() ERROR: INVALID TYPE";
				std::cerr << std::endl;
				break;
		}
	}
}



/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/
/**************************** Groups **********************/


void PostedListDialog::groupInfoToGroupItemInfo(const RsGroupMetaData &groupInfo, GroupItemInfo &groupItemInfo)
{

}

void PostedListDialog::insertGroupData(const std::list<RsGroupMetaData> &groupList)
{

}

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/


