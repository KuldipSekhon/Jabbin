/*
 * jinglevoicecaller.cpp
 * Copyright (C) 2006  Remko Troncon
 * Minor fixes (C) 2008 Dmitriy Kazimirov,e-mail:dkazimirov@issart.com
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <qstring.h>
#include <qdom.h>

#include "talk/xmpp/constants.h"
#include "talk/base/sigslot.h"
#include "talk/xmpp/jid.h"
#include "talk/xmllite/xmlelement.h"
#include "talk/xmllite/xmlprinter.h"
#include "talk/base/network.h"
#include "talk/p2p/base/session.h"
#include "talk/p2p/base/sessionid.h"
#include "talk/p2p/base/sessionmanager.h"
//#include "talk/p2p/base/helpers.h"
#include "talk/base/helpers.h"
#include "talk/p2p/client/basicportallocator.h"
#include "talk/p2p/client/httpportallocator.h"
//#include "talk/p2p/client/sessionclient.h"
#include "talk/base/physicalsocketserver.h"
#include "talk/base/thread.h"
#include "talk/base/socketaddress.h"
#include "talk/session/phone/call.h"
#include "talk/session/phone/phonesessionclient.h"
//#include "talk/session/sessionsendtask.h"

#include "im.h"
#include "xmpp.h"
#include "xmpp_xmlcommon.h"
#include "jinglevoicecaller.h"
#include "psiaccount.h"
#include "dtmfsender.h"

#include "callslog.h"

// Should change in the future
#define JINGLE_NS "http://www.google.com/session"

// ----------------------------------------------------------------------------

class JingleIQResponder : public XMPP::Task
{
public:
	JingleIQResponder(XMPP::Task *);
	~JingleIQResponder();

	bool take(const QDomElement &);
};

/**
 * \class JingleIQResponder
 * \brief A task that responds to jingle candidate queries with an empty reply.
 */

JingleIQResponder::JingleIQResponder(Task *parent) :Task(parent)
{
}

JingleIQResponder::~JingleIQResponder()
{
}

bool JingleIQResponder::take(const QDomElement &e)
{
	if(e.tagName() != "iq")
		return false;

	QDomElement first = e.firstChild().toElement();
	if (!first.isNull() && first.attribute("xmlns") == JINGLE_NS) {
		QDomElement iq = createIQ(doc(), "result", e.attribute("from"), e.attribute("id"));
		send(iq);
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------

/**
 * \brief A class for handling signals from libjingle.
 */
class JingleClientSlots : public sigslot::has_slots<>
{
public:
	JingleClientSlots(JingleVoiceCaller *voiceCaller);

	void callCreated(cricket::Call *call);
	void callDestroyed(cricket::Call *call);
	//void sendStanza(cricket::SessionClient*, const buzz::XmlElement *stanza, bool);
	void sendStanza( const buzz::XmlElement *stanza );
	void requestSignaling();
	void stateChanged(cricket::Call *call, cricket::Session *session, cricket::Session::State state);

    void setJingleInfo(const std::string &relay_token,
		              const std::vector<std::string> &relay_addresses,
			          const std::vector<talk_base::SocketAddress> &stun_addresses);



private:
	JingleVoiceCaller* voiceCaller_;
};


JingleClientSlots::JingleClientSlots(JingleVoiceCaller *voiceCaller) : voiceCaller_(voiceCaller)
{
}

void JingleClientSlots::callCreated(cricket::Call *call)
{
	call->SignalSessionState.connect(this, &JingleClientSlots::stateChanged);
}

void JingleClientSlots::callDestroyed(cricket::Call *call)
{
	qDebug("JingleClientSlots: Call destroyed");
	if ( !call->sessions().size() ) return;

	Jid jid( call->sessions()[0]->remote_name().c_str() );
	if (voiceCaller_->calling(jid)) {
		qDebug(QString("Removing unterminated call to %1").arg(jid.full()));
		voiceCaller_->removeCall(jid);
		emit voiceCaller_->terminated(jid);
	}
}

/*
void JingleClientSlots::sendStanza(cricket::SessionClient*, const buzz::XmlElement *stanza, bool response )
{
	QString st(stanza->Str().c_str());
	st.replace("cli:iq","iq");
	st.replace(":cli=","=");
	//fprintf(stderr,"bling\n");
	voiceCaller_->sendStanza(st);
	//fprintf(stderr,"blong\n");
	//fprintf(stderr,"Sending stanza \n%s\n\n",st.latin1());
} */

void JingleClientSlots::sendStanza( const buzz::XmlElement *stanza )
{
	QString st( stanza->Str().c_str() );

    st.replace("cli:iq","iq");
	st.replace(":cli=","=");
	//fprintf(stderr,"bling\n");
	voiceCaller_->sendStanza(st);
	//fprintf(stderr,"blong\n");
	//fprintf(stderr,"Sending stanza \n%s\n\n",st.latin1());
}

void JingleClientSlots::requestSignaling()
{
	voiceCaller_->session_manager_->OnSignalingReady();
}

void JingleClientSlots::stateChanged(cricket::Call *call, cricket::Session *session, cricket::Session::State state)
{
    char* stateNames[] = {
        "STATE_INIT = 0",
        "STATE_SENTINITIATE, sent initiate, waiting for Accept or Reject",
        "STATE_RECEIVEDINITIATE, received an initiate. Call Accept or Reject",
        "STATE_SENTACCEPT, sent accept. begin connectivity establishment",
        "STATE_RECEIVEDACCEPT, received accept. begin connectivity establishment",
        "STATE_SENTMODIFY, sent modify, waiting for Accept or Reject",
        "STATE_RECEIVEDMODIFY, received modify, call Accept or Reject",
        "STATE_SENTREJECT, sent reject after receiving initiate",
        "STATE_RECEIVEDREJECT, received reject after sending initiate",
        "STATE_SENTREDIRECT, sent direct after receiving initiate",
        "STATE_SENTTERMINATE, sent terminate (any time / either side)",
        "STATE_RECEIVEDTERMINATE, received terminate (any time / either side)",
        "STATE_INPROGRESS, session accepted and in progress",
		"STATE_DEINIT, de-initializtion"
    };
	//TODO:dkzm:make correct handling of DEINIT
	//voiceCaller_->terminate()?
	qDebug(QString("jinglevoicecaller.cpp: State changed (%1):%2").arg(state).arg(stateNames[state]) );
	// Why is c_str() stuff needed to make it compile on OS X ?
	Jid jid(session->remote_name().c_str());


	if (state == cricket::Session::STATE_INIT) { }
	else if (state == cricket::Session::STATE_SENTINITIATE) {
		voiceCaller_->registerCall(jid,call);
	}
	else if (state == cricket::Session::STATE_RECEIVEDINITIATE) {
		voiceCaller_->registerCall(jid,call);
		emit voiceCaller_->incoming(jid);
	}
	else if (state == cricket::Session::STATE_SENTACCEPT) { }
	else if (state == cricket::Session::STATE_RECEIVEDACCEPT) {
		emit voiceCaller_->accepted(jid);
	}
	else if (state == cricket::Session::STATE_SENTMODIFY) { }
	else if (state == cricket::Session::STATE_RECEIVEDMODIFY) {
		qWarning(QString("jinglevoicecaller.cpp: RECEIVEDMODIFY not implemented yet (was from %1)").arg(jid.full()));
	}
	else if (state == cricket::Session::STATE_SENTREJECT) { }
	else if (state == cricket::Session::STATE_RECEIVEDREJECT) {
		voiceCaller_->removeCall(jid);
		emit voiceCaller_->rejected(jid);
	}
	else if (state == cricket::Session::STATE_SENTREDIRECT) { }
	else if (state == cricket::Session::STATE_SENTTERMINATE) {
		voiceCaller_->removeCall(jid);
		emit voiceCaller_->terminated(jid);
	}
	else if (state == cricket::Session::STATE_RECEIVEDTERMINATE) {
		voiceCaller_->removeCall(jid);
		emit voiceCaller_->terminated(jid);
	}
	else if (state == cricket::Session::STATE_INPROGRESS) {
		emit voiceCaller_->in_progress(jid);
	}
}

void JingleClientSlots::setJingleInfo(const std::string &relay_token,
		              const std::vector<std::string> &relay_addresses,
                      const std::vector<talk_base::SocketAddress> &stun_addresses)
{
  voiceCaller_->port_allocator_->SetStunHosts(stun_addresses);
  voiceCaller_->port_allocator_->SetRelayHosts(relay_addresses);
  voiceCaller_->port_allocator_->SetRelayToken(relay_token);
}


// ----------------------------------------------------------------------------

/**
 * \class JingleVoiceCaller
 * \brief A Voice Calling implementation using libjingle.
 */

JingleVoiceCaller::JingleVoiceCaller(PsiAccount* acc) : VoiceCaller(acc)
{
	qDebug() << "Creating JingleVoiceCaller";
	initialized_ = false;
}

void JingleVoiceCaller::initialize()
{
	qDebug() << "Initialising JingleVoiceCaller";
	if (initialized_)
		return;

	QString jid = ((ClientStream&) account()->client()->stream()).jid().full();
	qDebug(QString("jinglevoicecaller.cpp: Creating new caller for %1").arg(jid));
	if (jid.isEmpty()) {
		qWarning("jinglevoicecaller.cpp: Empty JID");
		return;
	}

    std::string s_jid = std::string(jid.utf8().data(), jid.utf8().length());
	buzz::Jid j(s_jid);

	cricket::InitRandom( j.Str().c_str(), j.Str().size() );

	// Global variables
	if ( !socket_server_ ) {
		socket_server_ = new talk_base::PhysicalSocketServer();
		talk_base::Thread *t = new talk_base::Thread( socket_server_ );
		talk_base::ThreadManager::SetCurrent(t);
		t->Start();
		thread_ = t;

		//DKZM NOTE:why THIS server?
		//It's one of gtalk's (libjingle default)
        stun_addr_ = new talk_base::SocketAddress("64.233.167.126",19302);

        //stun_addr_ = new cricket::SocketAddress("192.245.12.229",19302); // stunserver.org
		//stun_addr_ = new cricket::SocketAddress("stun.fwdnet.net",19302);
		network_manager_ = new talk_base::NetworkManager();

        /*port_allocator_ = new cricket::BasicPortAllocator(network_manager_,
                                                          stun_addr_,
                                                          NULL); // relay server
        */
        port_allocator_ = new cricket::HttpPortAllocator( network_manager_, "call" );

        std::vector<talk_base::SocketAddress> stun_addresses;
        stun_addresses.push_back( talk_base::SocketAddress("64.233.167.126",19302) );

        port_allocator_->SetStunHosts(stun_addresses);

	}

	slots_ = new JingleClientSlots(this);

	// Session manager
	session_manager_ = new cricket::SessionManager( port_allocator_, thread_ );
	session_manager_->SignalRequestSignaling.connect(slots_, &JingleClientSlots::requestSignaling);
	session_manager_->OnSignalingReady();

    session_manager_->SignalOutgoingMessage.connect(slots_, &JingleClientSlots::sendStanza );

    /*
    buzz::JingleInfoTask *jit = new buzz::JingleInfoTask(xmpp_client_);
    jit->RefreshJingleInfoNow();
    jit->SignalJingleInfo.connect( slots_, &JingleClientSlots::setJingleInfo);
    jit->Start();
    */


	// Phone Client
	phone_client_ = new cricket::PhoneSessionClient( j, session_manager_ );
	phone_client_->SignalCallCreate.connect( slots_, &JingleClientSlots::callCreated );
	phone_client_->SignalCallDestroy.connect( slots_, &JingleClientSlots::callDestroyed );

    //phone_client_->SignalSendStanza.connect(slots_, &JingleClientSlots::sendStanza);

	// IQ Responder
	new JingleIQResponder(account()->client()->rootTask());

	// Listen to incoming packets
	connect(account()->client(),SIGNAL(xmlIncoming(const QString&)),SLOT(receiveStanza(const QString&)));

//	FIXME!!!! Change log system based on Psi
//    CallsLog *callsLog = new CallsLog( SPathDir::instance()->historyDir() + "/tins_calls.log", this );

	initialized_ = true;
}


void JingleVoiceCaller::deinitialize()
{
	if (!initialized_)
		return;

	// Stop listening to incoming packets
	disconnect(account()->client(),SIGNAL(xmlIncoming(const QString&)),this,SLOT(receiveStanza(const QString&)));

	// Disconnect signals (is this needed)
	//phone_client_->SignalCallCreate.disconnect(slots_);
	//phone_client_->SignalSendStanza.disconnect(slots_);

	// Delete objects
	delete phone_client_;
	delete session_manager_;
	delete slots_;

    phone_client_ = 0;
    session_manager_ = 0;
    slots_ = 0;

	initialized_ = false;
}


JingleVoiceCaller::~JingleVoiceCaller()
{
    deinitialize();
}

bool JingleVoiceCaller::calling(const Jid& jid)
{
	return calls_.contains(jid.full());
}

void JingleVoiceCaller::call(const Jid& jid)
{
    if ( !initialized_)
        return;

    qDebug(QString("jinglevoicecaller.cpp: Calling %1").arg(jid.full()));
	cricket::Call *c = ((cricket::PhoneSessionClient*)(phone_client_))->CreateCall();
	std::string s_jid_full = std::string(jid.full().utf8().data(), jid.full().utf8().length());
    c->InitiateSession( buzz::Jid( s_jid_full ), 0 );
	phone_client_->SetFocus(c);
}

void JingleVoiceCaller::accept(const Jid& j)
{
	qDebug("jinglevoicecaller.cpp: Accepting call");
	cricket::Call* call = calls_[j.full()];
	if (call != NULL) {
		call->AcceptSession(call->sessions()[0]);
		phone_client_->SetFocus(call);
	}
}

void JingleVoiceCaller::reject(const Jid& j)
{
	qDebug("jinglevoicecaller.cpp: Rejecting call");
	cricket::Call* call = calls_[j.full()];
	if (call != NULL) {
		call->RejectSession(call->sessions()[0]);
		calls_.remove(j.full());
	}
}

void JingleVoiceCaller::terminate(const Jid& j)
{
	qDebug(QString("jinglevoicecaller.cpp: Terminating call to %1").arg(j.full()));
	cricket::Call* call = calls_[j.full()];
	if (call != NULL) {
		call->Terminate();
		calls_.remove(j.full());
	}
}

void JingleVoiceCaller::sendDTMF(const Jid& j, QString dtmfCode )
{
    cricket::Call* call = calls_[j.full()];
	if (call != NULL) {
        cricket::Session* session = call->sessions()[0];
        QString sid = session->id().id_str().c_str();
        QString from = ((ClientStream&) account()->client()->stream()).jid().full();
        QString to = session->remote_name().c_str();
        QString initiator = from;

        DTMFSender *sender = new DTMFSender(this,sid,from,to,initiator);
        sender->sendDTMF( dtmfCode, account()->client() );
    }
}

void JingleVoiceCaller::sendStanza(const char* stanza)
{
    QString s = stanza;
    s = s.replace("><",">\n<");
    qDebug( QString("Send stanza:\n%1\n\n").arg(s) );

    account()->client()->send(QString(stanza));
}

void JingleVoiceCaller::registerCall(const Jid& jid, cricket::Call* call)
{
	qDebug("jinglevoicecaller.cpp: Registering call\n");
	if (!calls_.contains(jid.full())) {
		calls_[jid.full()] = call;
	}
	else {
		qWarning("jinglevoicecaller.cpp: Auto-rejecting call because another call is currently open");
		call->RejectSession(call->sessions()[0]);
	}
}

void JingleVoiceCaller::removeCall(const Jid& j)
{
	qDebug(QString("JingleVoiceCaller: Removing call to %1").arg(j.full()));
	calls_.remove(j.full());
}

void JingleVoiceCaller::receiveStanza(const QString& stanza)
{
    QDomDocument doc;
	doc.setContent(stanza);

	// Check if it is offline presence from an open chat
	if (doc.documentElement().tagName() == "presence") {
		Jid from = Jid(doc.documentElement().attribute("from"));
		QString type = doc.documentElement().attribute("type");
		if (type == "unavailable" && calls_.contains(from.full())) {
			qDebug("JingleVoiceCaller: User went offline without closing a call.");
			removeCall(from);
			emit terminated(from);
		}
		return;
	}

	// Check if the packet is destined for libjingle.
	// We could use Session::IsClientStanza to check this, but this one crashes
	// for some reason.
	QDomNode n = doc.documentElement().firstChild();
	bool ok = false;
	while (!n.isNull() && !ok) {
		QDomElement e = n.toElement();
		if (!e.isNull() && e.attribute("xmlns") == JINGLE_NS) {
			ok = true;
		}
		n = n.nextSibling();
	}

	// Spread the word
	if (ok) {
		qDebug(QString("jinglevoicecaller.cpp: Handing down \n%1").arg(stanza));
        std::string s_stanza = std::string(stanza.utf8().data(), stanza.utf8().length());
		buzz::XmlElement *e = buzz::XmlElement::ForStr(s_stanza);

        //phone_client_->OnIncomingStanza(e);
        session_manager_->OnIncomingMessage( e );
	}
}

talk_base::SocketServer* JingleVoiceCaller::socket_server_ = NULL;
talk_base::Thread* JingleVoiceCaller::thread_ = NULL;
talk_base::NetworkManager* JingleVoiceCaller::network_manager_ = NULL;
cricket::HttpPortAllocator* JingleVoiceCaller::port_allocator_ = NULL;
talk_base::SocketAddress* JingleVoiceCaller::stun_addr_ = NULL;
