// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "LoginContainer.h"
#include "WebLoginWidget.h"
#include "ClassicLoginWidget.h"

#include <QtUiTools>
#include <QFile>
#include <QHash>
#include <QMessageBox>

#include "UiModule.h"
#include "UiProxyWidget.h"
#include "UiWidgetProperties.h"

namespace CoreUi
{
    LoginContainer::LoginContainer(
        Foundation::Framework *framework,
        RexLogic::OpenSimLoginHandler *os_login_handler,
        RexLogic::TaigaLoginHandler *taiga_login_handler) :
        framework_(framework),
        os_login_handler_(os_login_handler),
        taiga_login_handler_(taiga_login_handler),
        login_widget_(0), 
        login_progress_widget_(0), 
        progress_bar_timer_(0), 
        autohide_timer_(0), 
        login_progress_bar_(0), 
        login_status_(0)
    {
        ui_services_ = framework_->GetModuleManager()->GetModule<UiServices::UiModule>(Foundation::Module::MT_UiServices);
        if (ui_services_.lock().get())
        {
            InitLoginUI(os_login_handler_, taiga_login_handler_);
            QObject::connect(this, SIGNAL( DisconnectSignal() ), os_login_handler_, SLOT( Logout() ));
        }
        else
        {
            UiServices::UiModule::LogDebug("CoreUi::LoginContainer >> Could not accuire UiServices module, skipping UI creation");
        }
    }

    LoginContainer::~LoginContainer()
    {
        SAFE_DELETE(classic_login_widget_);
        SAFE_DELETE(web_login_widget_);
        SAFE_DELETE(tabWidget_);
        SAFE_DELETE(login_widget_);
        login_proxy_widget_ = 0;
    }

    void LoginContainer::Connected()
    {
        login_is_in_progress_ = false;

        SAFE_DELETE(progress_bar_timer_);
        SAFE_DELETE(login_progress_bar_);
        SAFE_DELETE(login_status_);
        SAFE_DELETE(login_progress_widget_);
        login_progress_proxy_widget_ = 0;
    }

    void LoginContainer::QuitApplication()
    {
        emit( QuitApplicationSignal() );
    }

    void LoginContainer::StartParameterLoginTaiga(QString &server_entry_point_url)
    {
        emit( CommandParameterLogin(server_entry_point_url) );
    }

    void LoginContainer::StartParameterLoginOpenSim(QString &first_and_last, QString &password, QString &server_address_with_port)
    {
        if (!server_address_with_port.startsWith("http://"))
            server_address_with_port = "http://" + server_address_with_port;

        QMap<QString, QString> map;
        map["AuthType"] = "OpenSim";
        map["Username"] = first_and_last;
        map["Password"] = password;
        map["WorldAddress"] = server_address_with_port;
        
        emit( DisconnectSignal() );
        emit( CommandParameterLogin(map) );
    }

    void LoginContainer::StartParameterLoginRealXtend(QString &username, QString &password, QString &authAddressWithPort, QString &server_address_with_port)
    {
        if (!server_address_with_port.startsWith("http://"))
            server_address_with_port = "http://" + server_address_with_port;
        if (!authAddressWithPort.startsWith("http://"))
            authAddressWithPort = "http://" + authAddressWithPort;

        QMap<QString, QString> map;
        map["AuthType"] = "RealXtend";
        map["Username"] = username;
        map["Password"] = password;
        map["WorldAddress"] = server_address_with_port;
        map["AuthenticationAddress"] = authAddressWithPort;
        
        emit( DisconnectSignal() );
        emit( CommandParameterLogin(map) );
    }

    void LoginContainer::InitLoginUI(RexLogic::OpenSimLoginHandler *os_login_handler, RexLogic::TaigaLoginHandler *taiga_login_handler)
    {
        boost::shared_ptr<UiServices::UiModule> uiServices = ui_services_.lock();
        QUiLoader loader;
        QFile uiFile("./data/ui/login/login_controller.ui");

        if ( uiServices.get() && uiFile.exists() )
        {
            login_widget_ = loader.load(&uiFile);
           
            message_frame_ = login_widget_->findChild<QFrame *>("messageFrame");
            hide_message_button_ = login_widget_->findChild<QPushButton *>("hideButton");
            message_label_ = login_widget_->findChild<QLabel *>("messageLabel");
            autohide_label_ = login_widget_->findChild<QLabel *>("autohideLabel");
            tabWidget_ = login_widget_->findChild<QTabWidget *>("tabWidget");
            
            message_frame_->hide();
            message_label_->setText("");
            autohide_label_->setText("");

            tabWidget_->clear();
            classic_login_widget_ = new ClassicLoginWidget(this, os_login_handler, framework_);
            web_login_widget_ = new WebLoginWidget(this, taiga_login_handler);
            tabWidget_->addTab(classic_login_widget_, "Login");
            tabWidget_->addTab(web_login_widget_, "Web Login");
                      
            login_proxy_widget_ = uiServices->GetSceneManager()->AddWidgetToCurrentScene(login_widget_, UiServices::UiWidgetProperties("Login", true));
            login_is_in_progress_ = false;

            QObject::connect(hide_message_button_, SIGNAL( clicked() ), this, SLOT( HideMessageFromUser() ));
            QObject::connect(this, SIGNAL( QuitApplicationSignal() ), os_login_handler, SLOT( Quit() ));
        }
    }

    void LoginContainer::StartLoginProgressUI()
    {
        boost::shared_ptr<UiServices::UiModule> uiServices = ui_services_.lock();
        QUiLoader loader;
        QFile uiFile("./data/ui/login/login_progress.ui");

        if ( uiServices.get() && uiFile.exists() )
        {
            login_progress_widget_ = loader.load(&uiFile);
            login_progress_widget_->hide();
            login_status_ = login_progress_widget_->findChild<QLabel *>("statusLabel");
            login_progress_bar_ = login_progress_widget_->findChild<QProgressBar *>("progressBar");
            uiFile.close();

            login_progress_proxy_widget_ = uiServices->GetSceneManager()->AddWidgetToCurrentScene(login_progress_widget_, UiServices::UiWidgetProperties("Login loader", true, UiServices::SlideFromBottom));
            login_progress_proxy_widget_->show();
            login_is_in_progress_ = true;
        }
    }

    void LoginContainer::HideLoginProgressUI()
    {
        if (progress_bar_timer_)
        {
            progress_bar_timer_->stop();
            SAFE_DELETE(progress_bar_timer_);
        }

        SAFE_DELETE(login_progress_bar_);
        SAFE_DELETE(login_status_);
        SAFE_DELETE(login_progress_widget_);
        login_progress_proxy_widget_ = 0;
    }

    void LoginContainer::UpdateLoginProgressUI(const QString &status, int progress, const ProtocolUtilities::Connection::State connection_state)
    {
        if (connection_state != ProtocolUtilities::Connection::STATE_ENUM_COUNT)
        {
            if (connection_state == ProtocolUtilities::Connection::STATE_INIT_XMLRPC)
            {
                login_status_->setText("Initialising connection");
                AnimateProgressBar(5);
            }
            else if (connection_state == ProtocolUtilities::Connection::STATE_XMLRPC_AUTH_REPLY_RECEIVED)
            {
                login_status_->setText("Authentication reply received");
                AnimateProgressBar(13);
            }
            else if (connection_state == ProtocolUtilities::Connection::STATE_WAITING_FOR_XMLRPC_REPLY)
            {
                login_status_->setText("Waiting for servers response...");
                AnimateProgressBar(26);
            }
            else if (connection_state == ProtocolUtilities::Connection::STATE_XMLRPC_REPLY_RECEIVED)
            {
                login_status_->setText("Login response received");
                AnimateProgressBar(50);
            }
            else if (connection_state == ProtocolUtilities::Connection::STATE_INIT_UDP)
            {
                login_status_->setText("Creating World Stream...");
                AnimateProgressBar(50); 
            }
            CreateProgressTimer(250);
        }
        else
        {
            if (progress == 100)
            {
                progress_bar_timer_->stop();
                Connected();
                return;
            }
            login_status_->setText(status);
            AnimateProgressBar(progress);
            CreateProgressTimer(500);
        }
    }

    void LoginContainer::AnimateProgressBar(int new_value)
    {
        if (new_value > login_progress_bar_->value() && new_value != 100)
            for (int i=login_progress_bar_->value(); i<=new_value; i++)
                login_progress_bar_->setValue(i);
    }

    void LoginContainer::UpdateProgressBar()
    {
        if (login_progress_bar_)
        {
            int oldvalue = login_progress_bar_->value();
            if (oldvalue < 99)
            {
                if (oldvalue > 79 && progress_bar_timer_->interval() != 1000)
                {
                    login_status_->setText("Downloading world objects...");
                    CreateProgressTimer(1000);
                }
                else if (oldvalue > 90 && progress_bar_timer_->interval() != 3500)
                {
                    CreateProgressTimer(3500);
                }
                login_progress_bar_->setValue(++oldvalue);
            }
        }
    }

    void LoginContainer::CreateProgressTimer(int interval)
    {
        if (progress_bar_timer_)
            progress_bar_timer_->stop();
        SAFE_DELETE(progress_bar_timer_);
        progress_bar_timer_ = new QTimer();
        QObject::connect(progress_bar_timer_, SIGNAL( timeout() ), this, SLOT( UpdateProgressBar() ));
        progress_bar_timer_->start(interval);
    }

    void LoginContainer::ShowMessageToUser(QString message, int autohide_seconds)
    {
        login_is_in_progress_ = false;

        HideLoginProgressUI();
        message_label_->setText(message);
        message_frame_->show();

        if (autohide_timer_)
            autohide_timer_->stop();
        autohide_timer_ = new QTimer(this);
        QObject::connect(autohide_timer_, SIGNAL( timeout() ), this, SLOT( UpdateAutoHide() ));
        autohide_count_ = autohide_seconds;
        autohide_timer_->start(1000);
    }

    void LoginContainer::HideMessageFromUser()
    {
        autohide_timer_->stop();
        message_frame_->hide();
        message_label_->setText("");
        autohide_label_->setText("");
        autohide_count_ = 0;
    }

    void LoginContainer::UpdateAutoHide()
    {
        if ( autohide_count_ > 0 )
        {
            autohide_label_->setText(QString("Autohide in %1").arg(QString::number(autohide_count_)));
            --autohide_count_;
        }
        else
            HideMessageFromUser();
    }
}
