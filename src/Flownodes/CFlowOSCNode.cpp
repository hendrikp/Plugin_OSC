/* OSC_Plugin - for licensing and copyright see license.txt */

#include <StdAfx.h>
#include <Nodes/G2FlowBaseNode.h>

#include <oscpkt/oscpkt.hh>
#include <oscpkt/udp.hh>

#include <list>
#include <map>

#include <CPluginOSC.h>

using namespace oscpkt;

namespace OSCPlugin
{
    class COSCConnection;

    std::map<int, COSCConnection* > g_OSCConnections;
    int g_nFreeConnection = 1;

    COSCConnection& GetConnection( int nConnection )
    {
        assert( g_OSCConnections.find( nConnection ) != g_OSCConnections.end() );
        assert( g_OSCConnections[nConnection] != 0 );

        return *g_OSCConnections[nConnection];
    }

    enum eOSCType
    {
        OSCT_String,
        OSCT_Float32,
        OSCT_Double64,
        OSCT_Int32,
        OSCT_Int64,
        OSCT_Bool,
        OSCT_Any,
    };

    template<typename T1>
    T1 InitOSCType()
    {
        return 0;
    };

    template<>
    string InitOSCType<string>()
    {
        return "";
    };

    template<typename T1>
    eOSCType ToOSCType()
    {
        return OSCT_Any;
    };

    template<>
    eOSCType ToOSCType<string>()
    {
        return OSCT_String;
    };

    template<>
    eOSCType ToOSCType<int>()
    {
        return OSCT_Int32;
    };

    template<>
    eOSCType ToOSCType<long long>()
    {
        return OSCT_Int64;
    };

    template<>
    eOSCType ToOSCType<float>()
    {
        return OSCT_Float32;
    };

    template<>
    eOSCType ToOSCType<double>()
    {
        return OSCT_Double64;
    };

    template<>
    eOSCType ToOSCType<bool>()
    {
        return OSCT_Bool;
    };

    struct SOSCValueInfo
    {
        eOSCType type;
        TFlowGraphId graphid;
        SFlowAddress address;

        SOSCValueInfo( eOSCType _type, TFlowGraphId _id, SFlowAddress& _address ) :
            type( _type ),
            graphid( _id ),
            address( _address )
        {
        };
    };

    class ISendInfo
    {
        public:
            virtual void Send( PacketWriter& pw ) const = 0;
            virtual void Release() = 0;

            virtual void AddValue( SOSCValueInfo& info ) { };
    };

    class IReceiveInfo
    {
        public:
            virtual void Receive( Message& msg ) const = 0;
            virtual void Release() = 0;
    };

    class COSCBundleStart : public ISendInfo
    {
        public:
            void Send( PacketWriter& pw ) const
            {
                pw.startBundle();
            };

            void Release()
            {
                delete this;
            };
    };

    class COSCBundleEnd : public ISendInfo
    {
        public:
            void Send( PacketWriter& pw ) const
            {
                pw.endBundle();
            };

            void Release()
            {
                delete this;
            };
    };

    class COSCMessage : public ISendInfo, public IReceiveInfo
    {
            std::string m_sMessage;
            std::list<SOSCValueInfo> m_OSCValues;

        public:
            COSCMessage( string sMessage )
            {
                m_sMessage = sMessage.c_str();
            }

            void AddValue( SOSCValueInfo& info )
            {
                m_OSCValues.push_back( info );
            }

            void Send( PacketWriter& pw ) const
            {
                Message msg( m_sMessage );
                std::list<SOSCValueInfo>::const_iterator iter;

                for ( iter = m_OSCValues.begin(); iter != m_OSCValues.end(); ++iter )
                {
                    const TFlowInputData* data = gEnv->pFlowSystem->GetGraphById( ( *iter ).graphid )->GetInputValue( ( *iter ).address.node, ( *iter ).address.port );
                    bool bRet = false;

                    switch ( ( *iter ).type )
                    {
                        case OSCT_String:
                            {
                                string dat;
                                bRet = data->GetValueWithConversion( dat );
                                msg.pushStr( dat.c_str() );
                                break;
                            }

                        case OSCT_Int32:
                            {
                                int dat;
                                bRet = data->GetValueWithConversion( dat );
                                msg.pushInt32( dat );
                                break;
                            }

                        case OSCT_Int64:
                            {
                                long long dat;
                                int dat2;
                                bRet = data->GetValueWithConversion( dat2 );
                                dat = dat2;
                                msg.pushInt64( dat );
                                break;
                            }

                        case OSCT_Float32:
                            {
                                float dat;
                                bRet = data->GetValueWithConversion( dat );
                                msg.pushFloat( dat );
                                break;
                            }

                        case OSCT_Double64:
                            {
                                double dat;
                                float dat2;
                                bRet = data->GetValueWithConversion( dat2 );
                                dat = dat2;
                                msg.pushDouble( dat );
                                break;
                            }

                        case OSCT_Bool:
                            {
                                bool dat;
                                bRet = data->GetValueWithConversion( dat );
                                msg.pushBool( dat );
                                break;
                            }
                    }
                }

                pw.addMessage( msg );
            };

            void Receive( Message& msg ) const
            {
                if ( msg.match( m_sMessage ) )
                {
                    std::list<SOSCValueInfo>::const_iterator iter;

                    Message::ArgReader arg( msg );

                    for ( iter = m_OSCValues.begin(); iter != m_OSCValues.end(); ++iter )
                    {
                        if ( arg.nbArgRemaining() && arg.isOk() )
                        {
                            switch ( ( *iter ).type )
                            {
                                case OSCT_Any:
                                    {
                                        arg.pop();
                                        break;
                                    }

                                case OSCT_String:
                                    {
                                        if ( arg.isStr() )
                                        {
                                            std::string dat;
                                            arg.popStr( dat );
                                            gEnv->pFlowSystem->GetGraphById( ( *iter ).graphid )->ActivatePort( ( *iter ).address, string( dat.c_str() ) );
                                            break;
                                        }

                                        goto WrongType;
                                    }

                                case OSCT_Int32:
                                    {
                                        if ( arg.isInt32() )
                                        {
                                            int dat;
                                            arg.popInt32( dat );
                                            gEnv->pFlowSystem->GetGraphById( ( *iter ).graphid )->ActivatePort( ( *iter ).address, dat );
                                            break;
                                        }

                                        goto WrongType;
                                    }

                                case OSCT_Int64:
                                    {
                                        if ( arg.isInt64() )
                                        {
                                            long long dat;
                                            int dat2;
                                            arg.popInt64( dat );
                                            dat2 = dat;
                                            gEnv->pFlowSystem->GetGraphById( ( *iter ).graphid )->ActivatePort( ( *iter ).address, dat2 );
                                            break;
                                        }

                                        goto WrongType;
                                    }

                                case OSCT_Float32:
                                    {
                                        if ( arg.isFloat() )
                                        {
                                            float dat;
                                            arg.popFloat( dat );
                                            gEnv->pFlowSystem->GetGraphById( ( *iter ).graphid )->ActivatePort( ( *iter ).address, dat );
                                            break;
                                        }

                                        goto WrongType;
                                    }

                                case OSCT_Double64:
                                    {
                                        if ( arg.isDouble() )
                                        {
                                            double dat;
                                            float dat2;
                                            arg.popDouble( dat );
                                            dat2 = dat;
                                            gEnv->pFlowSystem->GetGraphById( ( *iter ).graphid )->ActivatePort( ( *iter ).address, dat2 );
                                            break;
                                        }

                                        goto WrongType;
                                    }

                                case OSCT_Bool:
                                    {
                                        if ( arg.isBool() )
                                        {
                                            bool dat;
                                            arg.popBool( dat );
                                            gEnv->pFlowSystem->GetGraphById( ( *iter ).graphid )->ActivatePort( ( *iter ).address, dat );
                                            break;
                                        }

                                        goto WrongType;
                                    }
                            }
                        }
                    }
                }

                return;
WrongType:
                gPlugin->LogWarning( "message %s received unexpected type", m_sMessage.c_str() );
            };

            void Release()
            {
                delete this;
            };
    };

    class COSCPacket
    {
            bool m_bAutoSend;
            bool m_bSend;
            std::vector<ISendInfo*> m_Content;

        public:
            COSCPacket()
            {
                m_bSend = false;
                m_bAutoSend = true;
            }

            ~COSCPacket()
            {
                std::vector<ISendInfo*>::const_iterator iter;

                for ( iter = m_Content.begin(); iter != m_Content.end(); ++iter )
                {
                    ( *iter )->Release();
                }
            }

            void SetAutoSend( bool bAutoSend )
            {
                m_bAutoSend = true;
            }

            void NotifyChange()
            {
                if ( m_bAutoSend )
                {
                    m_bSend = true;
                }
            }

            void RequestSend()
            {
                m_bSend = true;
            }

            int AddContent( ISendInfo* content )
            {
                m_Content.push_back( content );
                return m_Content.size() - 1;
            }

            ISendInfo& GetISendInfo( int nMessage )
            {
                assert( nMessage >= 0 );
                assert( nMessage < m_Content.size() );

                return *m_Content[nMessage];
            }

            bool Send( UdpSocket& sock )
            {
                if ( m_bSend )
                {
                    m_bSend = false;
                    PacketWriter pw;

                    std::vector<ISendInfo*>::const_iterator iter;

                    for ( iter = m_Content.begin(); iter != m_Content.end(); ++iter )
                    {
                        ( *iter )->Send( pw );
                    }

                    return sock.sendPacket( pw.packetData(), pw.packetSize() );
                }

                return false;
            }
    };

    class COSCConnection
    {
            UdpSocket m_sock;

            std::vector<COSCMessage> m_ReceiveOSCMessages;
            std::vector<COSCPacket> m_Packets;
            int m_nConnection;

        public:
            COSCConnection()
            {
                m_nConnection = g_nFreeConnection++;
                g_OSCConnections[m_nConnection] = this;
            }

            ~COSCConnection()
            {
                Reset();
                g_OSCConnections.erase( m_nConnection );
            }

            int GetId()
            {
                return m_nConnection;
            }

            void Reset()
            {
                m_sock.close();
                m_ReceiveOSCMessages.clear();
                m_Packets.clear();
            }

            int AddReceiveMessage( string sMessage )
            {
                m_ReceiveOSCMessages.push_back( COSCMessage( sMessage ) );
                return m_ReceiveOSCMessages.size() - 1;
            }

            COSCMessage& GetReceiveMessage( int nMessage )
            {
                assert( nMessage >= 0 );
                assert( nMessage < m_ReceiveOSCMessages.size() );

                return m_ReceiveOSCMessages[nMessage];
            }

            int AddPacket()
            {
                m_Packets.push_back( COSCPacket() );
                return m_Packets.size() - 1;
            }

            COSCPacket& GetPacket( int nPacket )
            {
                assert( nPacket >= 0 );
                assert( nPacket < m_Packets.size() );

                return m_Packets[nPacket];
            }

            bool Connect( string sHost, int nPort, bool bServer )
            {
                Reset();

                if ( bServer )
                {
                    m_sock.bindTo( nPort );
                }

                else
                {
                    m_sock.connectTo( std::string( sHost.c_str() ), nPort );
                }

                if ( m_sock.isOk() )
                {
                    gPlugin->LogAlways( "Socket connected port %d", nPort );
                    return true;
                }

                else
                {
                    gPlugin->LogError( "Error connection to port %d", nPort );
                    return false;
                }
            }

            void Update()
            {
                if ( m_sock.isOk() )
                {
                    // Receive Data
                    while ( m_sock.receiveNextPacket( 0 ) )
                    {
                        PacketReader pr( m_sock.packetData(), m_sock.packetSize() );
                        Message* incoming_msg;

                        while ( pr.isOk() && ( incoming_msg = pr.popMessage() ) != 0 )
                        {
                            for ( std::vector<COSCMessage>::const_iterator iter = m_ReceiveOSCMessages.begin(); iter != m_ReceiveOSCMessages.end(); ++iter )
                            {
                                ( *iter ).Receive( *incoming_msg );
                            }
                        }
                    }

                    // Send Data
                    for ( std::vector<COSCPacket>::iterator iter = m_Packets.begin(); m_sock.isOk() && iter != m_Packets.end(); ++iter )
                    {
                        ( *iter ).Send( m_sock );
                    }
                }

                else
                {
                    gPlugin->LogError( "Sock error: %s - is the server running?", m_sock.errorMessage().c_str() );
                }
            }
    };

    class CFlowConnectionNode :
        public CFlowBaseNode<eNCT_Instanced>
    {
            enum EInputPorts
            {
                EIP_INIT = 0,
                EIP_CLOSE,
                EIP_HOST,
                EIP_PORT,
                EIP_TYPE,
            };

            enum EOutputPorts
            {
                EOP_NEXTINIT = 0,
            };

#define INITIALIZE_OUTPUTS(x) \
    ActivateOutput(x, EOP_NEXTINIT, Vec3(-1,-1,-1));\
     
            enum EConnectionType
            {
                CT_Client = 0,
                CT_Server,
                CT_Default = CT_Client,
            };

            COSCConnection m_conn;

        public:
            virtual void GetMemoryUsage( ICrySizer* s ) const
            {
                s->Add( *this );
            }

            virtual IFlowNodePtr Clone( SActivationInfo* pActInfo )
            {
                return new CFlowConnectionNode( pActInfo );
            }

            CFlowConnectionNode( SActivationInfo* pActInfo )
            {

            }

            virtual void GetConfiguration( SFlowNodeConfig& config )
            {
                static const SInputPortConfig inputs[] =
                {
                    InputPortConfig_Void( "Init", _HELP( "Connect / Initialize" ) ),
                    InputPortConfig_Void( "Close", _HELP( "Disconnect / Close" ) ),

                    InputPortConfig<string>( "sHost", "localhost", _HELP( "host/ip to bind/connect" ), "sHost", _UICONFIG( "" ) ),
                    InputPortConfig<int>( "nPort", 7777, _HELP( "port to listen/connect" ), "nPort", _UICONFIG( "" ) ),
                    InputPortConfig<int>( "nType", int( CT_Default ), _HELP( "type" ), "nType", _UICONFIG( "enum_int:UDP-Client=0,UDP-Server=1" ) ),
                    InputPortConfig_Null(),
                };

                static const SOutputPortConfig outputs[] =
                {
                    OutputPortConfig<Vec3>( "InitAllRMessageOrSPacket", _HELP( "for initialization of all send packets and receive messages" ) ),
                    OutputPortConfig_Null(),
                };

                config.pInputPorts = inputs;
                config.pOutputPorts = outputs;
                config.sDescription = _HELP( PLUGIN_CONSOLE_PREFIX " Connection" );

                config.SetCategory( EFLN_APPROVED );
            }

            virtual void ProcessEvent( EFlowEvent evt, SActivationInfo* pActInfo )
            {
                switch ( evt )
                {
                    case eFE_Suspend:
                        pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
                        break;

                    case eFE_Resume:
                        pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
                        break;

                    case eFE_Initialize:
                        INITIALIZE_OUTPUTS( pActInfo );
                        break;

                    case eFE_Activate:
                        if ( IsPortActive( pActInfo, EIP_CLOSE ) )
                        {
                            INITIALIZE_OUTPUTS( pActInfo );
                            pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
                        }

                        if ( IsPortActive( pActInfo, EIP_INIT ) )
                        {
                            m_conn.Connect( GetPortString( pActInfo, EIP_HOST ), GetPortInt( pActInfo, EIP_PORT ), ( bool )GetPortInt( pActInfo, EIP_TYPE ) );

                            ActivateOutput( pActInfo, EOP_NEXTINIT, Vec3( m_conn.GetId(), -1, -1 ) );
                            pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
                        }

                        break;

                    case eFE_Update:
                        m_conn.Update();
                        break;
                }
            }
    };

    class CFlowReceiveValueNodeAny :
        public CFlowBaseNode<eNCT_Instanced>
    {
            enum EInputPorts
            {
                EIP_INIT = 0,
            };

            enum EOutputPorts
            {
                EOP_NEXTINIT = 0,
            };

        public:
            virtual void GetMemoryUsage( ICrySizer* s ) const
            {
                s->Add( *this );
            }

            virtual IFlowNodePtr Clone( SActivationInfo* pActInfo )
            {
                return new CFlowReceiveValueNodeAny( pActInfo );
            }

            CFlowReceiveValueNodeAny( SActivationInfo* pActInfo )
            {

            }

            virtual void GetConfiguration( SFlowNodeConfig& config )
            {
                static const SInputPortConfig inputs[] =
                {
                    InputPortConfig<Vec3>( "InitFromRMessageOrRValue", _HELP( "Initialize" ) ),
                    InputPortConfig_Null(),
                };

                static const SOutputPortConfig outputs[] =
                {
                    OutputPortConfig<Vec3>( "InitNextRValue", _HELP( "for initialization of next receive value" ) ),
                    OutputPortConfig_Null(),
                };

                config.pInputPorts = inputs;
                config.pOutputPorts = outputs;
                config.sDescription = _HELP( PLUGIN_CONSOLE_PREFIX " Receive Any" );

                config.SetCategory( EFLN_APPROVED );
            }

            virtual void ProcessEvent( EFlowEvent evt, SActivationInfo* pActInfo )
            {
                switch ( evt )
                {
                    case eFE_Activate:

                        if ( IsPortActive( pActInfo, EIP_INIT ) )
                        {
                            Vec3 initializer = GetPortVec3( pActInfo, EIP_INIT );
                            GetConnection( initializer[0] ).GetReceiveMessage( initializer[2] ).AddValue(
                                SOSCValueInfo(
                                    OSCT_Any,
                                    pActInfo->pGraph->GetGraphId(),
                                    SFlowAddress( pActInfo->myID,
                                                  -1,
                                                  true
                                                )
                                )
                            );

                            ActivateOutput( pActInfo, EOP_NEXTINIT, initializer );
                        }

                        break;
                }
            }
    };

    template<typename T1, eOSCType T2>
    class CFlowReceiveValueNode :
        public CFlowBaseNode<eNCT_Instanced>
    {
            enum EInputPorts
            {
                EIP_INIT = 0,
            };

            enum EOutputPorts
            {
                EOP_NEXTINIT = 0,
                EOP_VALUE,
            };

        public:
            virtual void GetMemoryUsage( ICrySizer* s ) const
            {
                s->Add( *this );
            }

            virtual IFlowNodePtr Clone( SActivationInfo* pActInfo )
            {
                return new CFlowReceiveValueNode( pActInfo );
            }

            CFlowReceiveValueNode( SActivationInfo* pActInfo )
            {

            }

            virtual void GetConfiguration( SFlowNodeConfig& config )
            {
                static const SInputPortConfig inputs[] =
                {
                    InputPortConfig<Vec3>( "InitFromRMessageOrRValue", _HELP( "Initialize" ) ),
                    InputPortConfig_Null(),
                };

                static const SOutputPortConfig outputs[] =
                {
                    OutputPortConfig<Vec3>( "InitNextRValue", _HELP( "for initialization of next receive value" ) ),
                    OutputPortConfig<T1>( "Value", _HELP( "value" ) ),
                    OutputPortConfig_Null(),
                };

                config.pInputPorts = inputs;
                config.pOutputPorts = outputs;
                config.sDescription = _HELP( PLUGIN_CONSOLE_PREFIX " Receive Type" );

                config.SetCategory( EFLN_APPROVED );
            }

            virtual void ProcessEvent( EFlowEvent evt, SActivationInfo* pActInfo )
            {
                switch ( evt )
                {
                    case eFE_Activate:

                        if ( IsPortActive( pActInfo, EIP_INIT ) )
                        {
                            Vec3 initializer = GetPortVec3( pActInfo, EIP_INIT );
                            GetConnection( initializer[0] ).GetReceiveMessage( initializer[2] ).AddValue(
                                SOSCValueInfo( T2, //ToOSCType<T1>(),
                                               pActInfo->pGraph->GetGraphId(),
                                               SFlowAddress( pActInfo->myID,
                                                             EOP_VALUE,
                                                             true
                                                           )
                                             )
                            );

                            ActivateOutput( pActInfo, EOP_NEXTINIT, initializer );
                        }

                        break;
                }
            }
    };

    class CFlowReceiveMessageNode :
        public CFlowBaseNode<eNCT_Instanced>
    {
            enum EInputPorts
            {
                EIP_INIT = 0,
                EIP_MESSAGE,
            };

            enum EOutputPorts
            {
                EOP_NEXTINIT = 0,
            };

        public:
            virtual void GetMemoryUsage( ICrySizer* s ) const
            {
                s->Add( *this );
            }

            virtual IFlowNodePtr Clone( SActivationInfo* pActInfo )
            {
                return new CFlowReceiveMessageNode( pActInfo );
            }

            CFlowReceiveMessageNode( SActivationInfo* pActInfo )
            {

            }

            virtual void GetConfiguration( SFlowNodeConfig& config )
            {
                static const SInputPortConfig inputs[] =
                {
                    InputPortConfig<Vec3>( "InitFromConnection", _HELP( "Initialize" ) ),
                    InputPortConfig<string>( "sMessage", "/", _HELP( "Message" ), "sMessage", _UICONFIG( "" ) ),
                    InputPortConfig_Null(),
                };

                static const SOutputPortConfig outputs[] =
                {
                    OutputPortConfig<Vec3>( "InitNextRValue", _HELP( "for initialization of first receive value" ) ),
                    OutputPortConfig_Null(),
                };

                config.pInputPorts = inputs;
                config.pOutputPorts = outputs;
                config.sDescription = _HELP( PLUGIN_CONSOLE_PREFIX " Receive Message" );

                config.SetCategory( EFLN_APPROVED );
            }

            virtual void ProcessEvent( EFlowEvent evt, SActivationInfo* pActInfo )
            {
                switch ( evt )
                {
                    case eFE_Activate:

                        if ( IsPortActive( pActInfo, EIP_INIT ) )
                        {
                            Vec3 initializer = GetPortVec3( pActInfo, EIP_INIT );
                            initializer[2] = GetConnection( initializer[0] ).AddReceiveMessage( GetPortString( pActInfo, EIP_MESSAGE ) );
                            ActivateOutput( pActInfo, EOP_NEXTINIT, initializer );
                        }

                        break;
                }
            }
    };

    class CFlowSendPacketNode :
        public CFlowBaseNode<eNCT_Instanced>
    {
            enum EInputPorts
            {
                EIP_INIT = 0,
                EIP_SEND,
                EIP_AUTOSEND,
            };

            enum EOutputPorts
            {
                EOP_NEXTINIT = 0,
            };

            int m_pkt;
        public:
            virtual void GetMemoryUsage( ICrySizer* s ) const
            {
                s->Add( *this );
            }

            virtual IFlowNodePtr Clone( SActivationInfo* pActInfo )
            {
                return new CFlowSendPacketNode( pActInfo );
            }

            CFlowSendPacketNode( SActivationInfo* pActInfo )
            {
                m_pkt = -1;
            }

            virtual void GetConfiguration( SFlowNodeConfig& config )
            {
                static const SInputPortConfig inputs[] =
                {
                    InputPortConfig<Vec3>( "InitFromConnection", _HELP( "Initialize" ) ),
                    InputPortConfig_Void( "Send", _HELP( "Send" ) ),
                    InputPortConfig<bool>( "bAutoSend", true, _HELP( "Activate automatic sending on value change inside this packet" ) ),
                    InputPortConfig_Null(),
                };

                static const SOutputPortConfig outputs[] =
                {
                    OutputPortConfig<Vec3>( "InitNextSMessageOrSBundle", _HELP( "for initialization of first bundle/message." ) ),
                    OutputPortConfig_Null(),
                };

                config.pInputPorts = inputs;
                config.pOutputPorts = outputs;
                config.sDescription = _HELP( PLUGIN_CONSOLE_PREFIX " Send Packet" );

                config.SetCategory( EFLN_APPROVED );
            }

            virtual void ProcessEvent( EFlowEvent evt, SActivationInfo* pActInfo )
            {
                switch ( evt )
                {
                    case eFE_Activate:
                        Vec3 initializer = GetPortVec3( pActInfo, EIP_INIT );

                        if ( IsPortActive( pActInfo, EIP_INIT ) )
                        {
                            initializer[1] = m_pkt = GetConnection( initializer[0] ).AddPacket();

                            GetConnection( initializer[0] ).GetPacket( m_pkt ).SetAutoSend( GetPortBool( pActInfo, EIP_AUTOSEND ) );

                            ActivateOutput( pActInfo, EOP_NEXTINIT, initializer );
                        }

                        if ( IsPortActive( pActInfo, EIP_AUTOSEND ) && initializer[0] > 0 && m_pkt >= 0 )
                        {
                            GetConnection( initializer[0] ).GetPacket( m_pkt ).SetAutoSend( GetPortBool( pActInfo, EIP_AUTOSEND ) );
                        }

                        if ( IsPortActive( pActInfo, EIP_SEND ) && initializer[0] > 0 && m_pkt >= 0 )
                        {
                            GetConnection( initializer[0] ).GetPacket( m_pkt ).RequestSend();
                        }

                        break;
                }
            }
    };

    class CFlowSendMessageNode :
        public CFlowBaseNode<eNCT_Instanced>
    {
            enum EInputPorts
            {
                EIP_INIT = 0,
                EIP_MESSAGE,
            };

            enum EOutputPorts
            {
                EOP_NEXTINIT = 0,
            };

        public:
            virtual void GetMemoryUsage( ICrySizer* s ) const
            {
                s->Add( *this );
            }

            virtual IFlowNodePtr Clone( SActivationInfo* pActInfo )
            {
                return new CFlowSendMessageNode( pActInfo );
            }

            CFlowSendMessageNode( SActivationInfo* pActInfo )
            {

            }

            virtual void GetConfiguration( SFlowNodeConfig& config )
            {
                static const SInputPortConfig inputs[] =
                {
                    InputPortConfig<Vec3>( "InitFromSBundleOrSPacketOrSValue", _HELP( "Initialize" ) ),
                    InputPortConfig<string>( "sMessage", "/", _HELP( "Message" ), "sMessage", _UICONFIG( "" ) ),
                    InputPortConfig_Null(),
                };

                static const SOutputPortConfig outputs[] =
                {
                    OutputPortConfig<Vec3>( "InitNextSValueOrSBundle", _HELP( "for initialization of next send value or bundle" ) ),
                    OutputPortConfig_Null(),
                };

                config.pInputPorts = inputs;
                config.pOutputPorts = outputs;
                config.sDescription = _HELP( PLUGIN_CONSOLE_PREFIX " Send Message" );

                config.SetCategory( EFLN_APPROVED );
            }

            virtual void ProcessEvent( EFlowEvent evt, SActivationInfo* pActInfo )
            {
                switch ( evt )
                {
                    case eFE_Activate:
                        Vec3 initializer = GetPortVec3( pActInfo, EIP_INIT );

                        if ( IsPortActive( pActInfo, EIP_INIT ) )
                        {
                            initializer[2] = GetConnection( initializer[0] ).GetPacket( initializer[1] ).AddContent( new COSCMessage( GetPortString( pActInfo, EIP_MESSAGE ) ) );
                            ActivateOutput( pActInfo, EOP_NEXTINIT, initializer );
                        }

                        break;
                }
            }
    };

    template<typename T1, eOSCType T2>
    class CFlowSendValueNode :
        public CFlowBaseNode<eNCT_Instanced>
    {
            enum EInputPorts
            {
                EIP_INIT = 0,
                EIP_VALUE,
            };

            enum EOutputPorts
            {
                EOP_NEXTINIT = 0,
            };

            T1 m_value;

        public:
            virtual void GetMemoryUsage( ICrySizer* s ) const
            {
                s->Add( *this );
            }

            virtual IFlowNodePtr Clone( SActivationInfo* pActInfo )
            {
                return new CFlowSendValueNode( pActInfo );
            }

            CFlowSendValueNode( SActivationInfo* pActInfo )
            {
                m_value = InitOSCType<T1>();
            }

            virtual void GetConfiguration( SFlowNodeConfig& config )
            {
                static const SInputPortConfig inputs[] =
                {
                    InputPortConfig<Vec3>( "InitFromSMessageOrSValue", _HELP( "Initialize" ) ),
                    InputPortConfig<T1>( "Value", _HELP( "value" ) ),
                    InputPortConfig_Null(),
                };

                static const SOutputPortConfig outputs[] =
                {
                    OutputPortConfig<Vec3>( "InitNextSValueOrSMessageOrSBundle", _HELP( "for initialization of next send value or message or bundle" ) ),
                    OutputPortConfig_Null(),
                };

                config.pInputPorts = inputs;
                config.pOutputPorts = outputs;
                config.sDescription = _HELP( PLUGIN_CONSOLE_PREFIX " Send Type" );

                config.SetCategory( EFLN_APPROVED );
            }

            virtual void ProcessEvent( EFlowEvent evt, SActivationInfo* pActInfo )
            {
                switch ( evt )
                {
                    case eFE_Activate:
                        Vec3 initializer = GetPortVec3( pActInfo, EIP_INIT );

                        if ( IsPortActive( pActInfo, EIP_INIT ) )
                        {
                            GetConnection( initializer[0] ).GetPacket( initializer[1] ).GetISendInfo( initializer[2] ).AddValue(
                                SOSCValueInfo( T2,
                                               pActInfo->pGraph->GetGraphId(),
                                               SFlowAddress( pActInfo->myID,
                                                             EIP_VALUE,
                                                             false
                                                           )
                                             ) );

                            ActivateOutput( pActInfo, EOP_NEXTINIT, initializer );
                        }

                        if ( IsPortActive( pActInfo, EIP_VALUE ) && initializer[0] > 0 && initializer[1] >= 0 )
                        {
                            const TFlowInputData& val = GetPortAny( pActInfo, EIP_VALUE );
                            T1 curval;

                            if ( val.GetValueWithConversion( curval ) )
                            {
                                if ( m_value != curval )
                                {
                                    m_value = curval;
                                    GetConnection( initializer[0] ).GetPacket( initializer[1] ).NotifyChange();
                                }
                            }
                        }

                        break;
                }
            }
    };

    class CFlowSendBundleStartNode :
        public CFlowBaseNode<eNCT_Instanced>
    {
            enum EInputPorts
            {
                EIP_INIT = 0,
            };

            enum EOutputPorts
            {
                EOP_NEXTINIT = 0,
            };

        public:
            virtual void GetMemoryUsage( ICrySizer* s ) const
            {
                s->Add( *this );
            }

            virtual IFlowNodePtr Clone( SActivationInfo* pActInfo )
            {
                return new CFlowSendBundleStartNode( pActInfo );
            }

            CFlowSendBundleStartNode( SActivationInfo* pActInfo )
            {

            }

            virtual void GetConfiguration( SFlowNodeConfig& config )
            {
                static const SInputPortConfig inputs[] =
                {
                    InputPortConfig<Vec3>( "InitFromSPacketOrSValueOrSBundle", _HELP( "Initialize" ) ),
                    InputPortConfig_Null(),
                };

                static const SOutputPortConfig outputs[] =
                {
                    OutputPortConfig<Vec3>( "InitNextSValueOrSMessageOrSBundle", _HELP( "for initialization of next send message or bundle" ) ),
                    OutputPortConfig_Null(),
                };

                config.pInputPorts = inputs;
                config.pOutputPorts = outputs;
                config.sDescription = _HELP( PLUGIN_CONSOLE_PREFIX " Send Bundle Start" );

                config.SetCategory( EFLN_APPROVED );
            }

            virtual void ProcessEvent( EFlowEvent evt, SActivationInfo* pActInfo )
            {
                switch ( evt )
                {
                    case eFE_Activate:
                        Vec3 initializer = GetPortVec3( pActInfo, EIP_INIT );

                        if ( IsPortActive( pActInfo, EIP_INIT ) )
                        {
                            initializer[2] = -1;
                            GetConnection( initializer[0] ).GetPacket( initializer[1] ).AddContent( new COSCBundleStart() );
                            ActivateOutput( pActInfo, EOP_NEXTINIT, initializer );
                        }

                        break;
                }
            }
    };

    class CFlowSendBundleEndNode :
        public CFlowBaseNode<eNCT_Instanced>
    {
            enum EInputPorts
            {
                EIP_INIT = 0,
            };

            enum EOutputPorts
            {
                EOP_NEXTINIT = 0,
            };

        public:
            virtual void GetMemoryUsage( ICrySizer* s ) const
            {
                s->Add( *this );
            }

            virtual IFlowNodePtr Clone( SActivationInfo* pActInfo )
            {
                return new CFlowSendBundleEndNode( pActInfo );
            }

            CFlowSendBundleEndNode( SActivationInfo* pActInfo )
            {

            }

            virtual void GetConfiguration( SFlowNodeConfig& config )
            {
                static const SInputPortConfig inputs[] =
                {
                    InputPortConfig<Vec3>( "InitFromSPacketOrSValueOrSBundle", _HELP( "Initialize" ) ),
                    InputPortConfig_Null(),
                };

                static const SOutputPortConfig outputs[] =
                {
                    OutputPortConfig<Vec3>( "InitNextSValueOrSMessageOrSBundle", _HELP( "for initialization of next send message or bundle" ) ),
                    OutputPortConfig_Null(),
                };

                config.pInputPorts = inputs;
                config.pOutputPorts = outputs;
                config.sDescription = _HELP( PLUGIN_CONSOLE_PREFIX " Send Bundle End" );

                config.SetCategory( EFLN_APPROVED );
            }

            virtual void ProcessEvent( EFlowEvent evt, SActivationInfo* pActInfo )
            {
                switch ( evt )
                {
                    case eFE_Activate:
                        Vec3 initializer = GetPortVec3( pActInfo, EIP_INIT );

                        if ( IsPortActive( pActInfo, EIP_INIT ) )
                        {
                            initializer[2] = -1;
                            GetConnection( initializer[0] ).GetPacket( initializer[1] ).AddContent( new COSCBundleEnd() );
                            ActivateOutput( pActInfo, EOP_NEXTINIT, initializer );
                        }

                        break;
                }
            }
    };
}

REGISTER_FLOW_NODE_EX( "OSC_Plugin:Connection", OSCPlugin::CFlowConnectionNode, CFlowConnectionNode );

REGISTER_FLOW_NODE_EX( "OSC_Plugin:Receive:Message", OSCPlugin::CFlowReceiveMessageNode, CFlowReceiveMessageNode );
REGISTER_FLOW_NODE_EX( "OSC_Plugin:Receive:Value:Any", OSCPlugin::CFlowReceiveValueNodeAny, CFlowReceiveValueNodeAny );

typedef OSCPlugin::CFlowReceiveValueNode<int, OSCPlugin::OSCT_Int32> CFlowReceiveValueNodeInt32;
REGISTER_FLOW_NODE_EX( "OSC_Plugin:Receive:Value:Int32", CFlowReceiveValueNodeInt32, CFlowReceiveValueNodeInt32 );
typedef OSCPlugin::CFlowReceiveValueNode<int, OSCPlugin::OSCT_Int64> CFlowReceiveValueNodeInt64;
REGISTER_FLOW_NODE_EX( "OSC_Plugin:Receive:Value:Int64", CFlowReceiveValueNodeInt64, CFlowReceiveValueNodeInt64 );
typedef OSCPlugin::CFlowReceiveValueNode<float, OSCPlugin::OSCT_Float32> CFlowReceiveValueNodeFloat32;
REGISTER_FLOW_NODE_EX( "OSC_Plugin:Receive:Value:Float32", CFlowReceiveValueNodeFloat32, CFlowReceiveValueNodeFloat32 );
typedef OSCPlugin::CFlowReceiveValueNode<float, OSCPlugin::OSCT_Double64> CFlowReceiveValueNodeDouble64;
REGISTER_FLOW_NODE_EX( "OSC_Plugin:Receive:Value:Double64", CFlowReceiveValueNodeDouble64, CFlowReceiveValueNodeDouble64 );
typedef OSCPlugin::CFlowReceiveValueNode<string, OSCPlugin::OSCT_String> CFlowReceiveValueNodeString;
REGISTER_FLOW_NODE_EX( "OSC_Plugin:Receive:Value:String", CFlowReceiveValueNodeString, CFlowReceiveValueNodeString );
typedef OSCPlugin::CFlowReceiveValueNode<bool, OSCPlugin::OSCT_Bool> CFlowReceiveValueNodeBool;
REGISTER_FLOW_NODE_EX( "OSC_Plugin:Receive:Value:Bool", CFlowReceiveValueNodeBool, CFlowReceiveValueNodeBool );

REGISTER_FLOW_NODE_EX( "OSC_Plugin:Send:Packet", OSCPlugin::CFlowSendPacketNode, CFlowSendPacketNode );
REGISTER_FLOW_NODE_EX( "OSC_Plugin:Send:Message", OSCPlugin::CFlowSendMessageNode, CFlowSendMessageNode );
REGISTER_FLOW_NODE_EX( "OSC_Plugin:Send:BundleStart", OSCPlugin::CFlowSendBundleStartNode, CFlowSendBundleStartNode );
REGISTER_FLOW_NODE_EX( "OSC_Plugin:Send:BundleEnd", OSCPlugin::CFlowSendBundleEndNode, CFlowSendBundleEndNode );

typedef OSCPlugin::CFlowSendValueNode<int, OSCPlugin::OSCT_Int32> CFlowSendValueNodeInt32;
REGISTER_FLOW_NODE_EX( "OSC_Plugin:Send:Value:Int32", CFlowSendValueNodeInt32, CFlowSendValueNodeInt32 );
typedef OSCPlugin::CFlowSendValueNode<int, OSCPlugin::OSCT_Int64> CFlowSendValueNodeInt64;
REGISTER_FLOW_NODE_EX( "OSC_Plugin:Send:Value:Int64", CFlowSendValueNodeInt64, CFlowSendValueNodeInt64 );
typedef OSCPlugin::CFlowSendValueNode<float, OSCPlugin::OSCT_Float32> CFlowSendValueNodeFloat32;
REGISTER_FLOW_NODE_EX( "OSC_Plugin:Send:Value:Float32", CFlowSendValueNodeFloat32 , CFlowSendValueNodeFloat32 );
typedef OSCPlugin::CFlowSendValueNode<float, OSCPlugin::OSCT_Double64> CFlowSendValueNodeDouble64;
REGISTER_FLOW_NODE_EX( "OSC_Plugin:Send:Value:Double64", CFlowSendValueNodeDouble64 , CFlowSendValueNodeDouble64 );
typedef OSCPlugin::CFlowSendValueNode<string, OSCPlugin::OSCT_String>  CFlowSendValueNodeString;
REGISTER_FLOW_NODE_EX( "OSC_Plugin:Send:Value:String",  CFlowSendValueNodeString, CFlowSendValueNodeString );
typedef OSCPlugin::CFlowSendValueNode<bool, OSCPlugin::OSCT_Bool> CFlowSendValueNodeBool;
REGISTER_FLOW_NODE_EX( "OSC_Plugin:Send:Value:Bool", CFlowSendValueNodeBool, CFlowSendValueNodeBool );
