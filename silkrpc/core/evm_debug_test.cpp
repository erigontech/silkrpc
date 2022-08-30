/*
   Copyright 2021 The Silkrpc Authors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "evm_debug.hpp"

#include <string>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_future.hpp>
#include <catch2/catch.hpp>
#include <gmock/gmock.h>
#include <silkpre/precompile.h>

#include <silkrpc/common/log.hpp>
#include <silkrpc/common/util.hpp>
#include <silkrpc/core/rawdb/accessors.hpp>
#include <silkrpc/core/rawdb/chain.hpp>
#include <silkrpc/ethdb/tables.hpp>
#include <silkrpc/test/mock_database_reader.hpp>
#include <silkrpc/types/transaction.hpp>

namespace silkrpc::debug {

using Catch::Matchers::Message;
using evmc::literals::operator""_address;

using testing::_;
using testing::InvokeWithoutArgs;

static silkworm::Bytes kZeroKey{*silkworm::from_hex("0000000000000000")};
static silkworm::Bytes kZeroHeader{*silkworm::from_hex("bf7e331f7f7c1dd2e05159666b3bf8bc7a8a3a9eb1d518969eab529dd9b88c1a")};
static silkworm::Bytes kZeroAddress{*silkworm::from_hex("00000000000000000000")};

static silkworm::Bytes kConfigKey{
    *silkworm::from_hex("bf7e331f7f7c1dd2e05159666b3bf8bc7a8a3a9eb1d518969eab529dd9b88c1a")};
static silkworm::Bytes kConfigValue{*silkworm::from_hex(
    "7b22436861696e4e616d65223a22676f65726c69222c22636861696e4964223a352c22636f6e73656e737573223a22636c69717565222c2268"
    "6f6d657374656164426c6f636b223a302c2264616f466f726b537570706f7274223a747275652c22656970313530426c6f636b223a302c2265"
    "697031353048617368223a22307830303030303030303030303030303030303030303030303030303030303030303030303030303030303030"
    "303030303030303030303030303030303030303030222c22656970313535426c6f636b223a302c22656970313538426c6f636b223a302c2262"
    "797a616e7469756d426c6f636b223a302c22636f6e7374616e74696e6f706c65426c6f636b223a302c2270657465727362757267426c6f636b"
    "223a302c22697374616e62756c426c6f636b223a313536313635312c226265726c696e426c6f636b223a343436303634342c226c6f6e646f6e"
    "426c6f636b223a353036323630352c22636c69717565223a7b22706572696f64223a31352c2265706f6368223a33303030307d7d")};

TEST_CASE("DebugExecutor::execute precompiled") {
    SILKRPC_LOG_STREAMS(null_stream(), null_stream());
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    static silkworm::Bytes kAccountHistoryKey1{*silkworm::from_hex("0a6bb546b9208cfab9e8fa2b9b2c042b18df703000000000009db707")};
    static silkworm::Bytes kAccountHistoryKey2{*silkworm::from_hex("000000000000000000000000000000000000000900000000009db707")};
    static silkworm::Bytes kAccountHistoryKey3{*silkworm::from_hex("000000000000000000000000000000000000000000000000009db707")};

    static silkworm::Bytes kPlainStateKey1{*silkworm::from_hex("0a6bb546b9208cfab9e8fa2b9b2c042b18df7030")};
    static silkworm::Bytes kPlainStateKey2{*silkworm::from_hex("0000000000000000000000000000000000000009")};
    static silkworm::Bytes kPlainStateKey3{*silkworm::from_hex("000000000000000000000000000000000000000")};

    static silkworm::Bytes kPlainStateValue1{
        *silkworm::from_hex("0f010203ed03e8010520f1885eda54b7a053318cd41e2093220dab15d65381b1157a3633a83bfd5c9239")};

    test::MockDatabaseReader db_reader;
    boost::asio::thread_pool workers{1};

    ChannelFactory channel_factory = []() {
        return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
    };
    ContextPool context_pool{1, channel_factory};
    context_pool.start();

    SECTION("precompiled contract failure") {
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, silkworm::ByteView{kZeroKey}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return kZeroHeader;
            }));
        EXPECT_CALL(db_reader, get(db::table::kConfig, silkworm::ByteView{kConfigKey}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kConfigKey, kConfigValue};
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey1}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{silkworm::Bytes{}, silkworm::Bytes{}};
            }));
        EXPECT_CALL(db_reader, get_one(db::table::kPlainState, silkworm::ByteView{kPlainStateKey1}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return kPlainStateValue1;
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey2}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey2, silkworm::Bytes{}};
            }));
        EXPECT_CALL(db_reader, get_one(db::table::kPlainState, silkworm::ByteView{kPlainStateKey2}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return silkworm::Bytes{};
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey3}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey2, silkworm::Bytes{}};
            }));
        EXPECT_CALL(db_reader, get_one(db::table::kPlainState, silkworm::ByteView{kPlainStateKey3}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return silkworm::Bytes{};
            }));

        evmc::address max_precompiled{};
        max_precompiled.bytes[silkworm::kAddressLength - 1] = SILKPRE_NUMBER_OF_ISTANBUL_CONTRACTS;

        Call call;
        call.from = 0x0a6bb546b9208cfab9e8fa2b9b2c042b18df7030_address;
        call.to = max_precompiled;
        call.gas = 50'000;
        call.gas_price = 7;

        silkworm::Block block{};
        block.header.number = 10'336'006;

        boost::asio::io_context& io_context = context_pool.next_io_context();
        DebugExecutor executor{context_pool.next_io_context(), db_reader, workers};
        auto execution_result = boost::asio::co_spawn(io_context, executor.execute(block, call), boost::asio::use_future);
        const auto result = execution_result.get();

        context_pool.stop();
        context_pool.join();

        CHECK(!result.pre_check_error);
        CHECK(result.debug_trace == R"({
            "failed":true,
            "gas":50000,
            "returnValue":"",
            "structLogs":[]
        })"_json);
    }
}

TEST_CASE("DebugExecutor::execute call 1") {
    SILKRPC_LOG_STREAMS(null_stream(), null_stream());
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    static silkworm::Bytes kAccountHistoryKey1{*silkworm::from_hex("e0a2bd4258d2768837baa26a28fe71dc079f84c700000000005279a8")};
    static silkworm::Bytes kAccountHistoryValue1{*silkworm::from_hex(
        "0100000000000000000000003a300000010000005200c003100000008a5e905e9c5ea55ead5eb25eb75ebf5ec95ed25ed75ee15eed5ef25efa"
        "5eff5e085f115f1a5f235f2c5f355f3e5f475f505f595f625f6b5f745f7c5f865f8f5f985fa15faa5faf5fb45fb95fc15fce5fd75fe05fe65f"
        "f25ffb5f04600d6016601f602860306035603a6043604c6055605a606760706079607f608b6094609d60a560af60b860c160ca60d360db60e5"
        "60ee60f460fb600061096111611b6124612d6136613f61486151615a6160616c6175617e6187618f619961a261ab61b461bb61c661cc61d861"
        "e161ea61f361fc6102620e6217622062296230623a6241624d6256625f6271627a6283628c6295629e62a762b062b662c262ca62d462dc62e6"
        "62ef62f86201630a6313631c6325632e63376340634963526358635e6364636a6374637d6388639663a163ac63b563bc63c463ca63d063d963"
        "df63e563eb63f163fd6306640d641264186421642a6433643c6445644c64516457645d64646469647264776484648d6496649f64a864b164c3"
        "64cc64d164de64e764ee64f96402650b6514651d6526652f6538653d6547654d6553655c6565656e657765886592659b65a465ad65b665bf65"
        "c665cb65d165da65e365ec65f565fe6507661066196622662b6634663d6646664f6658666166676672667c6685668e669766a066a966b266bb"
        "66c466ca66d666df66e866f166f766fc6603670c6715671e6724673067386742674b67516757675d6766676f67786781678a678f6796679c67"
        "a167ae67b767c067c967d267e167ed67f667ff67086810681a682368296835683e684768506859685e686b6874687d6886688f689868a168aa"
        "68b368bc68c568ce68e068e968f268fb6804690d6916691f69266931693a6943694c6955695e6967697069796982698b6994699d69a669af69"
        "b869c169ca69d369dc69e569ee69f769fe69086a126a1b6a246a366a3f6a486a516a5a6a636a6c6a7e6a876a906a966aa26aab6ab46abd6ac6"
        "6acf6ad56adb6ae16aea6af36afc6a056b0e6b176b206b296b326b3b6b416b4b6b566b5c6b676b716b7a6b806b886b956b9e6ba76bb06bb96b"
        "bf6bc56bcb6bd06bd56bdd6be66bef6b016c0a6c136c1c6c226c2d6c346c406c496c526c5a6c646c6d6c766c7f6c886c916c966c9c6ca36cac"
        "6cb56cbe6cc76cd06cd96ce26ce86cf16cfd6c066d0f6d186d216d2a6d336d3c6d456d4e6d576d606d696d726d7b6d846d8a6d966d9f6da46d"
        "b16dba6dc36dcc6dd56dde6de76df06df76d026e096e146e1d6e266e2f6e386e416e4a6e516e5c6e656e6c6e746e806e896e906e9b6ea46ead"
        "6eb76ebf6ec86ed16eda6ee36eec6ef56efe6e076f0d6f196f226f2b6f346f3d6f466f4f6f586f616f666f706f776f7c6f856f8e6f976f9e6f"
        "a96fb26fb96fc46fcd6fd66fdc6fe36fe86ff16ffa6fff6f0c7015701e702670397042704b70527058705d7066706f70787081708a7093709a"
        "70a570ae70b770c070c970d170d970e470ed70f670fc7008711271197123712c7135713e714771597162716b7174717a7186718f719871a171"
        "aa71b371bc71c571ce71d771e071f271fb7104720d7216721f72287231723a7240724c7255725b7267726d7279728272897291729d72a672ac"
        "72b872c072ca72d372d972e572ee72f772007309731273197324732d7336733f73487351735a7363736c7375737a73877390739973a273ab73"
        "b473bd73c673cf73d873e173e773ee73f373fc7305740a7419742074297432743b7444744d7456745f746b747174797483748c7495749e74a7"
        "74b074b974c274cb74d074d774dd74e374ee74f87401750a7513751c7525752e7537754975527558755f7564756d7576757f75877591759a75"
        "a375aa75af75b575be75c775d075d975e275eb75f475fd750676187621762a7632763c7645764d7657766076697672767b7683768d7696769f"
        "76a876b176ba76c376cc76d576de76e776f076f97602770b7714771d7724772f77387741774a7753775c7765776e7774778077897792779b77"
        "a477aa77b177b677bf77c577d177da77e377e977f077f577fe7707781078197822782b7834783d7846784f78587861786a7873787c7885788e"
        "789778a078a878b178b878c478cd78d678df78e878f178fa7803790c7915791e7924793079387942794b7954795d7963796e79787981798979"
        "8e7993799879a579ab79b779c079c979d279db79e479ed79f679ff79087a117a1a7a237a2b7a357a3c7a447a507a597a627a6b7a747a7d7a86"
        "7a8f7a987aa17aaa7ab37abc7ac57ace7ad57ae07ae97aee7af87a017b0d7b167b1f7b287b2e7b377b3e7b437b4c7b557b5e7b677b707b797b"
        "827b8b7b947b9a7ba37baf7bb57bbc7bc17bca7bd37bdc7be47bea7bf57b007c097c0f7c197c217c2d7c367c3f7c487c517c5a7c637c6c7c75"
        "7c7e7c877c907c997ca17cab7cb27cb87cbd7cc67ccf7cd67ce17cea7cf37cfc7c057d0b7d177d207d297d317d3b7d417d4d7d537d5f7d657d"
        "6d7d777d837d8c7d957d9e7da77db07db97dc27dcb7dd27dd77ddd7de67def7df87d017e0a7e137e1c7e257e2e7e377e407e497e4f7e557e5b"
        "7e647e6a7e727e7f7e887e917e967ea37eac7eb57ebe7ec77ed07ed67ee27eeb7ef47efd7e037f0f7f187f217f2a7f337f3c7f457f4e7f577f"
        "607f667f727f7b7f847f8d7f")};

    static silkworm::Bytes kAccountHistoryKey2{*silkworm::from_hex("52728289eba496b6080d57d0250a90663a07e55600000000005279a8")};
    static silkworm::Bytes kAccountHistoryValue2{*silkworm::from_hex("0100000000000000000000003a300000010000004e00000010000000d63b")};

    static silkworm::Bytes kAccountHistoryKey3{*silkworm::from_hex("000000000000000000000000000000000000000000000000005279a8")};
    static silkworm::Bytes kAccountHistoryValue3{*silkworm::from_hex(
        "0100000000000000000000003b30270000000040202b003f002c001c002d0009002e000a002f000000300000003100000032000b003300"
        "0200340011003500030036000a003700040038000800390000003a0000003b0007003c0000003d000a003e0003003f0002004000060041"
        "000200420002004300010044000200450003004700050048000400490039004a0012004b0003004c0012004d00c2004e0010004f000500"
        "50007a0051001700520000005300650049010000c901000003020000170200002d0200002f02000031020000330200004b020000510200"
        "00750200007d020000930200009d020000af020000b1020000b3020000c3020000c5020000db020000e3020000e9020000f7020000fd02"
        "000003030000070300000d03000015030000210300002b0300009f030000c5030000cd030000f3030000790500009b050000a70500009d"
        "060000c3060000c5060000988d9b8d9c8d9d8d9f8da08da18da38da48da58da68da78da88da98dab8dac8dad8dae8daf8db08db18db28d"
        "b38db48db58dba8dbb8dbc8dbd8dbe8dbf8dc18dc28dc38dc48dc58dc68dc88dc98dca8d598fa7a2f6a2f9a207a344a3c8a331a446a423"
        "ad27ad37ae3cae40ae58ee5aee61eeb8eebeee44ef91ef9cef23f189f1c403ec033c047905b605d4120d133b147d147b168616641a5624"
        "c2cec6dce5dcd7df25e02ee071e093e0a2e00ae11de344e387e3a3e3abe37de43b249824413f5741734203549654a554bc5419db204529"
        "4530454c45d4abf0ab05ac0cac13ac18ac00b9dfb63f7fe3535bc76de078e080e088e095e09ce0a7e0aee0b4e0b8e0bde014431a4306a6"
        "d625e025ed25ff252e39ed3916722972497258725f7250735f738e749b74587c9b7c7da001657983a0d5a9d5c91fcd1f1a2046216d4975"
        "4a084bef6cf376418d8f8d113f4b49a1491a4db5e9ec542355a35c816b9a6cc3719e791c8909b4ce45f817bf4c074de94dfb4d154e1a4e"
        "1e4e714fa6b183bd84bd87bd8cbd8fbdc0bdabc0b8c0dbc0ebc011c5740543065c06630666436843754341754975f6a5a7ccf7e71aec2e"
        "fab12676415dfb73f280f287f2040f21369b5818863c86a5b2b4b2bab2afb4277fdf7ff27ff97fbd80cf808da643a80db4dbe3d2ff6511"
        "69116b116d116e1171117311d813f5138f149214c8142615411544156a1575157d157e15a415a515f31777448e44ba4d3155625b685b35"
        "5c425c585c465de15dd26b4f7250726072219328935d935e93a193e493e593e693ee93ef93f893fa931b941c94f3abb4aebeaeb2af6cb5"
        "fccc29cf09004cb4000037be000039be00003bbe09005dd1000060d1000062d100007fea010068eb0000af2b2442a79900d99e367b394d"
        "5fa17448c94dc98bcbe0cdf7cd74ce7dce86ceeecefece12cf30cf36cf3ccf49cf630a9c0a2025b93608500f5023502a502b5035503d50"
        "3e5043504b504d504e5054505650d250d750dd50e450e950f250f750fe5003510d6214621a621f6225622b62336235623a624162426247"
        "624c62556262626b6271627d6282628d6294629a62a162aa62b362b962bf62c462ca62ce62d662d762df62e762ee62f762ff6201630663"
        "0d6316631b632063286331633a6343634963506355635b6361636463706375637e63866387638f6394639a63a063a663ae63b463ba63c1"
        "63c663cb63d163db63e263e963ee63f363f863d78172947b948494899496949d94a494ad94b194b894be94c394cc94d594dd94e294e794"
        "ef94f694fb94029507950d9514951a951f9526952e9538953d9549954f9558955c9561956c95749577958095859592959795a395ad95b4"
        "95ba95bb95c195c895d195d695e395e895ee95f595fb95049610961696229628962e9634963d9646964f96549605975b97bd9714a234a7"
        "50c16fc501d80ad814d82cd841d84bd863d870d87cd84de3c3e989fb93fba7fbc9fefffe54ffdb07fb3f664f9c5099587d8a418b888be4"
        "8e2a90e49d91b59ddfd7e55be61de86ef3f1096579667a0a7def8bbcbb0b0f3b16974265537753895392539b53a453ad53b653d153da53"
        "e853f053f553fe530754105422542b5434543d5446544b54505461546a5473547c5485549754a054bb54c454ce54de54e654f154fa5403"
        "55095515551e55275542555d5578559c55a555aa55b355c055c555db55ed55f655fe550356085611561a5623562c5635563e5662566b56"
        "745686568f569856a156b356bc56c556ce56f256fb5604570a571f57315743574c575e577057795781578b5794579d57a657af57c157dc"
        "57e557f457fc57095819581b5824582d583b584858515862586c5875587e589958a058ab58b458bd58aa5a635dbc5dd65d568bc79a279b"
        "09000f770300147704001e770800287700003ba1000042be0000e2be0000fbbe0000edc500001b369f2b7444cf78de78327938793f7944"
        "794c79517957795c79637968796d79727979797e79837989798e79957996799d79aa79ab79b079b679bd79c279c779cf79d479da79db79"
        "e179e679ec79f379f879017a037a067a0d7a0e7a137a187a1f7a207a287a2d7a327a387a3d7a447a4a7a567a5b7a617a687afc7a017b08"
        "7b0e7b137b197b257b377b437b497b557b5f7b6a7b6c7b6d7b8d7b9f7ba97baf7bb57bbe7bc47bc57bd57bad7db37dbe7dbf7dd17de57d"
        "f17dfb7d017e0b7e157e207e287e2b7e397e4b7e517e5f7e")};

    static silkworm::Bytes kAccountChangeSetKey1{*silkworm::from_hex("00000000005279ab")};
    static silkworm::Bytes kAccountChangeSetSubkey1{*silkworm::from_hex("e0a2bd4258d2768837baa26a28fe71dc079f84c7")};
    static silkworm::Bytes kAccountChangeSetValue1{*silkworm::from_hex("030203430b141e903194951083c424fd")};

    static silkworm::Bytes kAccountChangeSetKey2{*silkworm::from_hex("0000000000532b9f")};
    static silkworm::Bytes kAccountChangeSetSubKey2{*silkworm::from_hex("0000000000000000000000000000000000000000")};
    static silkworm::Bytes kAccountChangeSetValue2{*silkworm::from_hex("020944ed67f28fd50bb8e9")};

    static silkworm::Bytes kPlainStateKey1{*silkworm::from_hex("e0a2bd4258d2768837baa26a28fe71dc079f84c7")};
    static silkworm::Bytes kPlainStateKey2{*silkworm::from_hex("52728289eba496b6080d57d0250a90663a07e556")};

    test::MockDatabaseReader db_reader;
    boost::asio::thread_pool workers{1};

    ChannelFactory channel_factory = []() {
        return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
    };
    ContextPool context_pool{1, channel_factory};
    context_pool.start();

    SECTION("Call: failed with intrinsic gas too low") {
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, silkworm::ByteView{kZeroKey}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return kZeroHeader;
            }));
        EXPECT_CALL(db_reader, get(db::table::kConfig, silkworm::ByteView{kConfigKey}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kConfigKey, kConfigValue};
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey1}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey1, kAccountHistoryValue1};
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey2}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey2, kAccountHistoryValue2};
            }));
        EXPECT_CALL(db_reader, get_one(db::table::kPlainState, silkworm::ByteView{kPlainStateKey1}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return silkworm::Bytes{};
            }));

        const auto block_number = 5'405'095; // 0x5279A7
        silkrpc::Call call;
        call.from = 0xe0a2Bd4258D2768837BAa26A28fE71Dc079f84c7_address;
        call.gas = 50'000;
        call.gas_price = 7;
        call.data = *silkworm::from_hex("602a60005500");

        silkworm::Block block{};
        block.header.number = block_number;

        boost::asio::io_context& io_context = context_pool.next_io_context();
        DebugExecutor executor{context_pool.next_io_context(), db_reader, workers};
        auto execution_result = boost::asio::co_spawn(io_context, executor.execute(block, call), boost::asio::use_future);
        auto result = execution_result.get();

        context_pool.stop();
        io_context.stop();
        context_pool.join();

        CHECK(result.pre_check_error.has_value() == true);
        CHECK(result.pre_check_error.value() == "tracing failed: intrinsic gas too low: have 50000, want 53072");
    }

    SECTION("Call: full output") {
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, silkworm::ByteView{kZeroKey}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return kZeroHeader;
            }));
        EXPECT_CALL(db_reader, get(db::table::kConfig, silkworm::ByteView{kConfigKey}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kConfigKey, kConfigValue};
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey1}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey1, kAccountHistoryValue1};
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey3}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey3, kAccountHistoryValue3};
            }));
        EXPECT_CALL(db_reader, get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey1}, silkworm::ByteView{kAccountChangeSetSubkey1}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
                co_return kAccountChangeSetValue1;
            }));
        EXPECT_CALL(db_reader, get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey2}, silkworm::ByteView{kAccountChangeSetSubKey2}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
                co_return kAccountChangeSetValue2;
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey2}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey2, kAccountHistoryValue2};
            }));
        EXPECT_CALL(db_reader, get_one(db::table::kPlainState, silkworm::ByteView{kPlainStateKey2}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return silkworm::Bytes{};
            }));

        const auto block_number = 5'405'095; // 0x5279A7
        silkrpc::Call call;
        call.from = 0xe0a2Bd4258D2768837BAa26A28fE71Dc079f84c7_address;
        call.gas = 118'936;
        call.gas_price = 7;
        call.data = *silkworm::from_hex("602a60005500");

        silkworm::Block block{};
        block.header.number = block_number;

        DebugExecutor executor{context_pool.next_io_context(), db_reader, workers};
        boost::asio::io_context& io_context = context_pool.next_io_context();
        auto execution_result = boost::asio::co_spawn(io_context.get_executor(), executor.execute(block, call), boost::asio::use_future);
        auto result = execution_result.get();

        context_pool.stop();
        io_context.stop();
        context_pool.join();

        CHECK(result.pre_check_error.has_value() == false);

        CHECK(result.debug_trace == R"({
            "failed": false,
            "gas": 75178,
            "returnValue": "",
            "structLogs": [
                {
                    "depth": 1,
                    "gas": 65864,
                    "gasCost": 3,
                    "memory": [],
                    "op": "PUSH1",
                    "pc": 0,
                    "stack": []
                },
                {
                    "depth": 1,
                    "gas": 65861,
                    "gasCost": 3,
                    "memory": [],
                    "op": "PUSH1",
                    "pc": 2,
                    "stack": [
                        "0x2a"
                    ]
                },
                {
                    "depth": 1,
                    "gas": 65858,
                    "gasCost": 22100,
                    "memory": [],
                    "op": "SSTORE",
                    "pc": 4,
                    "stack": [
                        "0x2a",
                        "0x0"
                    ],
                    "storage": {
                        "0000000000000000000000000000000000000000000000000000000000000000": "000000000000000000000000000000000000000000000000000000000000002a"
                    }
                },
                {
                    "depth": 1,
                    "gas": 43758,
                    "gasCost": 0,
                    "memory": [],
                    "op": "STOP",
                    "pc": 5,
                    "stack": []
                }
            ]
        })"_json);
    }

    SECTION("Call: no stack") {
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, silkworm::ByteView{kZeroKey}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return kZeroHeader;
            }));
        EXPECT_CALL(db_reader, get(db::table::kConfig, silkworm::ByteView{kConfigKey}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kConfigKey, kConfigValue};
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey1}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey1, kAccountHistoryValue1};
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey3}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey3, kAccountHistoryValue3};
            }));
        EXPECT_CALL(db_reader, get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey1}, silkworm::ByteView{kAccountChangeSetSubkey1}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
                co_return kAccountChangeSetValue1;
            }));
        EXPECT_CALL(db_reader, get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey2}, silkworm::ByteView{kAccountChangeSetSubKey2}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
                co_return kAccountChangeSetValue2;
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey2}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey2, kAccountHistoryValue2};
            }));
        EXPECT_CALL(db_reader, get_one(db::table::kPlainState, silkworm::ByteView{kPlainStateKey2}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return silkworm::Bytes{};
            }));

        const auto block_number = 5'405'095; // 0x5279A7
        silkrpc::Call call;
        call.from = 0xe0a2Bd4258D2768837BAa26A28fE71Dc079f84c7_address;
        call.gas = 118'936;
        call.gas_price = 7;
        call.data = *silkworm::from_hex("602a60005500");

        silkworm::Block block{};
        block.header.number = block_number;

        DebugConfig config{false, false, true};
        DebugExecutor executor{context_pool.next_io_context(), db_reader, workers, config};
        boost::asio::io_context& io_context = context_pool.next_io_context();
        auto execution_result = boost::asio::co_spawn(io_context.get_executor(), executor.execute(block, call), boost::asio::use_future);
        auto result = execution_result.get();

        context_pool.stop();
        io_context.stop();
        context_pool.join();

        CHECK(result.pre_check_error.has_value() == false);

        CHECK(result.debug_trace == R"({
            "failed": false,
            "gas": 75178,
            "returnValue": "",
            "structLogs": [
                {
                    "depth": 1,
                    "gas": 65864,
                    "gasCost": 3,
                    "memory": [],
                    "op": "PUSH1",
                    "pc": 0
                },
                {
                    "depth": 1,
                    "gas": 65861,
                    "gasCost": 3,
                    "memory": [],
                    "op": "PUSH1",
                    "pc": 2
                },
                {
                    "depth": 1,
                    "gas": 65858,
                    "gasCost": 22100,
                    "memory": [],
                    "op": "SSTORE",
                    "pc": 4,
                    "storage": {
                        "0000000000000000000000000000000000000000000000000000000000000000": "000000000000000000000000000000000000000000000000000000000000002a"
                    }
                },
                {
                    "depth": 1,
                    "gas": 43758,
                    "gasCost": 0,
                    "memory": [],
                    "op": "STOP",
                    "pc": 5
                }
            ]
        })"_json);
    }

    SECTION("Call: no memory") {
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, silkworm::ByteView{kZeroKey}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return kZeroHeader;
            }));
        EXPECT_CALL(db_reader, get(db::table::kConfig, silkworm::ByteView{kConfigKey}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kConfigKey, kConfigValue};
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey1}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey1, kAccountHistoryValue1};
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey3}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey3, kAccountHistoryValue3};
            }));
        EXPECT_CALL(db_reader, get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey1}, silkworm::ByteView{kAccountChangeSetSubkey1}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
                co_return kAccountChangeSetValue1;
            }));
        EXPECT_CALL(db_reader, get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey2}, silkworm::ByteView{kAccountChangeSetSubKey2}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
                co_return kAccountChangeSetValue2;
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey2}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey2, kAccountHistoryValue2};
            }));
        EXPECT_CALL(db_reader, get_one(db::table::kPlainState, silkworm::ByteView{kPlainStateKey2}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return silkworm::Bytes{};
            }));

        const auto block_number = 5'405'095; // 0x5279A7
        silkrpc::Call call;
        call.from = 0xe0a2Bd4258D2768837BAa26A28fE71Dc079f84c7_address;
        call.gas = 118'936;
        call.gas_price = 7;
        call.data = *silkworm::from_hex("602a60005500");

        silkworm::Block block{};
        block.header.number = block_number;

        DebugConfig config{false, true, false};
        DebugExecutor executor{context_pool.next_io_context(), db_reader, workers, config};
        boost::asio::io_context& io_context = context_pool.next_io_context();
        auto execution_result = boost::asio::co_spawn(io_context.get_executor(), executor.execute(block, call), boost::asio::use_future);
        auto result = execution_result.get();

        context_pool.stop();
        io_context.stop();
        context_pool.join();

        CHECK(result.pre_check_error.has_value() == false);

        CHECK(result.debug_trace == R"({
            "failed": false,
            "gas": 75178,
            "returnValue": "",
            "structLogs": [
                {
                    "depth": 1,
                    "gas": 65864,
                    "gasCost": 3,
                    "op": "PUSH1",
                    "pc": 0,
                    "stack": []
                },
                {
                    "depth": 1,
                    "gas": 65861,
                    "gasCost": 3,
                    "op": "PUSH1",
                    "pc": 2,
                    "stack": [
                        "0x2a"
                    ]
                },
                {
                    "depth": 1,
                    "gas": 65858,
                    "gasCost": 22100,
                    "op": "SSTORE",
                    "pc": 4,
                    "stack": [
                        "0x2a",
                        "0x0"
                    ],
                    "storage": {
                        "0000000000000000000000000000000000000000000000000000000000000000": "000000000000000000000000000000000000000000000000000000000000002a"
                    }
                },
                {
                    "depth": 1,
                    "gas": 43758,
                    "gasCost": 0,
                    "op": "STOP",
                    "pc": 5,
                    "stack": []
                }
            ]
        })"_json);
    }

    SECTION("Call: no storage") {
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, silkworm::ByteView{kZeroKey}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return kZeroHeader;
            }));
        EXPECT_CALL(db_reader, get(db::table::kConfig, silkworm::ByteView{kConfigKey}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kConfigKey, kConfigValue};
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey1}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey1, kAccountHistoryValue1};
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey3}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey3, kAccountHistoryValue3};
            }));
        EXPECT_CALL(db_reader, get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey1}, silkworm::ByteView{kAccountChangeSetSubkey1}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
                co_return kAccountChangeSetValue1;
            }));
        EXPECT_CALL(db_reader, get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey2}, silkworm::ByteView{kAccountChangeSetSubKey2}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
                co_return kAccountChangeSetValue2;
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey2}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey2, kAccountHistoryValue2};
            }));
        EXPECT_CALL(db_reader, get_one(db::table::kPlainState, silkworm::ByteView{kPlainStateKey2}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return silkworm::Bytes{};
            }));

        const auto block_number = 5'405'095; // 0x5279A7
        silkrpc::Call call;
        call.from = 0xe0a2Bd4258D2768837BAa26A28fE71Dc079f84c7_address;
        call.gas = 118'936;
        call.gas_price = 7;
        call.data = *silkworm::from_hex("602a60005500");

        silkworm::Block block{};
        block.header.number = block_number;

        DebugConfig config{true, false, false};
        DebugExecutor executor{context_pool.next_io_context(), db_reader, workers, config};
        boost::asio::io_context& io_context = context_pool.next_io_context();
        auto execution_result = boost::asio::co_spawn(io_context.get_executor(), executor.execute(block, call), boost::asio::use_future);
        auto result = execution_result.get();

        context_pool.stop();
        context_pool.join();

        CHECK(result.pre_check_error.has_value() == false);

        CHECK(result.debug_trace == R"({
            "failed": false,
            "gas": 75178,
            "returnValue": "",
            "structLogs": [
                {
                    "depth": 1,
                    "gas": 65864,
                    "gasCost": 3,
                    "memory": [],
                    "op": "PUSH1",
                    "pc": 0,
                    "stack": []
                },
                {
                    "depth": 1,
                    "gas": 65861,
                    "gasCost": 3,
                    "memory": [],
                    "op": "PUSH1",
                    "pc": 2,
                    "stack": [
                        "0x2a"
                    ]
                },
                {
                    "depth": 1,
                    "gas": 65858,
                    "gasCost": 22100,
                    "memory": [],
                    "op": "SSTORE",
                    "pc": 4,
                    "stack": [
                        "0x2a",
                        "0x0"
                    ]
                },
                {
                    "depth": 1,
                    "gas": 43758,
                    "gasCost": 0,
                    "memory": [],
                    "op": "STOP",
                    "pc": 5,
                    "stack": []
                }
            ]
        })"_json);
    }

    SECTION("Call: no stack, memory and storage") {
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, silkworm::ByteView{kZeroKey}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return kZeroHeader;
            }));
        EXPECT_CALL(db_reader, get(db::table::kConfig, silkworm::ByteView{kConfigKey}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kConfigKey, kConfigValue};
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey1}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey1, kAccountHistoryValue1};
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey3}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey3, kAccountHistoryValue3};
            }));
        EXPECT_CALL(db_reader, get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey1}, silkworm::ByteView{kAccountChangeSetSubkey1}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
                co_return kAccountChangeSetValue1;
            }));
        EXPECT_CALL(db_reader, get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey2}, silkworm::ByteView{kAccountChangeSetSubKey2}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
                co_return kAccountChangeSetValue2;
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey2}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                co_return KeyValue{kAccountHistoryKey2, kAccountHistoryValue2};
            }));
        EXPECT_CALL(db_reader, get_one(db::table::kPlainState, silkworm::ByteView{kPlainStateKey2}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                co_return silkworm::Bytes{};
            }));

        const auto block_number = 5'405'095; // 0x5279A7
        silkrpc::Call call;
        call.from = 0xe0a2Bd4258D2768837BAa26A28fE71Dc079f84c7_address;
        call.gas = 118'936;
        call.gas_price = 7;
        call.data = *silkworm::from_hex("602a60005500");

        silkworm::Block block{};
        block.header.number = block_number;

        DebugConfig config{true, true, true};
        DebugExecutor executor{context_pool.next_io_context(), db_reader, workers, config};
        boost::asio::io_context& io_context = context_pool.next_io_context();
        auto execution_result = boost::asio::co_spawn(io_context.get_executor(), executor.execute(block, call), boost::asio::use_future);
        auto result = execution_result.get();

        context_pool.stop();
        context_pool.join();

        CHECK(result.pre_check_error.has_value() == false);

        CHECK(result.debug_trace == R"({
            "failed": false,
            "gas": 75178,
            "returnValue": "",
            "structLogs": [
                {
                    "depth": 1,
                    "gas": 65864,
                    "gasCost": 3,
                    "op": "PUSH1",
                    "pc": 0
                },
                {
                    "depth": 1,
                    "gas": 65861,
                    "gasCost": 3,
                    "op": "PUSH1",
                    "pc": 2
                },
                {
                    "depth": 1,
                    "gas": 65858,
                    "gasCost": 22100,
                    "op": "SSTORE",
                    "pc": 4
                },
                {
                    "depth": 1,
                    "gas": 43758,
                    "gasCost": 0,
                    "op": "STOP",
                    "pc": 5
                }
            ]
        })"_json);
    }
}

TEST_CASE("DebugExecutor::execute call 2") {
    SILKRPC_LOG_STREAMS(null_stream(), null_stream());
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    static silkworm::Bytes kAccountHistoryKey1{*silkworm::from_hex("8ced5ad0d8da4ec211c17355ed3dbfec4cf0e5b900000000004366ad")};
    static silkworm::Bytes kAccountHistoryValue1{*silkworm::from_hex(
        "0100000000000000000000003a300000010000004300c00310000000d460da60de60f86008610d611161136125612d6149615c61626182"
        "619161bd61bf61cb61cd61d361e761e961f061f461f761f961026208620a620f621162176219621c621e621f622162246228622a622d62"
        "36623762386240624d625262536255625a625c625e62906296629b62a562a962ad62bc62be62c062c262ca62cf62d362dc62e362ea621b"
        "63216327632a63316338633d633f63426344634d634f6355636363656367636d63716373638563876389639b639c639f63a163a263a363"
        "bd63c263c863cb63cd63d663d863de63e163e6632c642f6430643b643e64406441644964526458645e64606464646864716474649464a6"
        "64ab64ac64cb64d164d464d564df64ee64f864fa64fb6409650c650e6510651165136536653c654565516564656f657265786590659465"
        "99659b65aa65af65b865bc65be65c465c665c765ca65cb65cc65cf65d065d265d465d765d965df65e565e865ea65ee65f365f465f965fa"
        "65fc65fe6500660166036605660b660c660f6614661766196620662166226624662566276629662a662d662f663066336638663a663d66"
        "3f66416645664c66516656665a665c665d665f6663666c6672667b66866688668a668d668f66926698669a669c669f66ac66ae66b166bb"
        "66c366c666c866ca66cc66cd66cf66d366db66dd66de66e066e366e566e966ec66ee66f566f666f766f966fb6600670267046706670867"
        "09670b670d6711671367146719671c672f673a6740674367446748674d675e67656769676b676c676e67706777677e6780678e67926795"
        "67a267ab67b467bc67c167db67e067e267e367e567e767ea67f167f66700681768196826682868306833683568376839683b683d683f68"
        "406848684d684f6852685368546857685b685d685e6865686b687068736877687a6880688e6890689b689f68a268b468bb68ca68ce68d0"
        "68db68f16803691069166921692569276929692c6937693c693e69416954695669586960696169646971698169a069bc69be69c069c169"
        "c369c469c569c669c969cb69cc69ce69cf69d169d269d369d669e269e469e769eb69f369f969fb69ff69006a036a046a056a076a0a6a0c"
        "6a0e6a116a216a2b6a386a3b6a406a626a9d6a9f6aa16aa36aa56aa76aa86aaa6aab6aac6aad6aaf6ab26ab36ab56ab66ab86ab96abb6a"
        "bc6abd6abf6ac06ac26ace6ad96ae06af56af66af86afd6a026b0a6b0d6b186b206b216b246b276b2b6b2c6b306b336b346b366b386b3a"
        "6b3c6b3e6b406b436b456b486b496b4a6b4c6b4e6b506b546b576b596b5b6b616b636b646b676b696b6b6b6d6b706b726b786b796b7a6b"
        "976ba86bab6bb06bd16bd76bd86bda6bdc6be16be66bee6bf06bf26b056c086c256c326c346c376c3d6c496c4e6c516c576c626c656c69"
        "6c6e6c7a6c7c6c826c876c8d6c926c956c976c9e6ca06ca26ca46ca96cb86cbf6cc26cdc6ce06ce26ce76cea6cef6cf16c026d236d2f6d"
        "326d346d4a6d686d6b6d716d896d8a6d8c6d8d6d916db56dba6dd26ddf6d2c6e6c6e796e9d6e9f6eb46e406f456f5b6f5c6f5e6f726f80"
        "6f826f836f856f876f9d6fab6fce6fd06ffe6f027007700b7014702f7038704c70667072708270867087708970a270a570a870aa70ac70"
        "b270b570b770b970ba70bc70bf70c270c570c770ca70cd70cf70ff70167117711a711c7123712c7131713d7147714d7153715a71607165"
        "71667168716a717f718971a071a771a971c971cd71cf71d071d171d271d371d471d671d771d871d971db71dc71f871fd71007201720372"
        "05720672087209720c720e721172127213721572177219721c721e722072227227722872297230723472377238723b723d723f72407244"
        "72477249728c728f729b72a072a372a872ac72ae72c272c672c872cb72cd72cf72d472dc72df72057306730a7323732d73327335733b73"
        "40734273447349734a734e735573587359735c735e735f7364736573667369736a736b73747385739373957396739d73a273af73b173b3"
        "73b973c073c873e273e473e773ee73f173f573267429742e743074337434748e749174937496749d749e74a174a474ac74b074b974c074"
        "c774c974cb74cd74d574d674d874d974da74de74e174e574fd74027505751a751d751f75227524752675277529752c7532753375347535"
        "754c7559755c756e757175737581758c759675ad75cb75e575e675f575f775f975007607760c7617764b765c7669766f76717672767476"
        "7576777678769276a776aa76ab76eb76f276067728773a773c774b777177a377b577db77ee77f077f477fb770678177826782c782e7833"
        "783a7841784578477849788178917895789878a478a778aa78ae78b178b478c578d278d378d678d778d878db78ee78f578177926793679"
        "40794d79507995799b79a079a379cf79ee79f4790a7a1b7a297a347a357a587a617a8e7a907a927a977a997a9a7aa07aa17aa67aa87aab"
        "7ac97acd7ad27ad77adb7add7ae47a0d7b377b6d7ba57ba67bb17bbd7bc07bc97bd57bd77bd97bda7bdd7bde7be07beb7bef7bf17bfb7b"
        "ff7b067c0d7c167c1d7c1f7c207c227c247c2c7c2e7c317c3d7c3f7c497c597c627c647c667c817c827c857c8d7c917c997c9b7c9c7c9f"
        "7ca47ca67ca87caa7cac7caf7cb37cc97ce57cf57c077d087d")};

    static silkworm::Bytes kAccountHistoryKey2{*silkworm::from_hex("5e1f0c9ddbe3cb57b80c933fab5151627d7966fa00000000004366ad")};
    static silkworm::Bytes kAccountHistoryValue2{*silkworm::from_hex(
            "0100000000000000000000003a300000020000004000020043000c00180000001e0000005e8d618d628d826688668d668f66a466ac"
            "66b866bb66cf66db6623678e67d167")};

    static silkworm::Bytes kAccountHistoryKey3{*silkworm::from_hex("000000000000000000000000000000000000000000000000004366ad")};
    static silkworm::Bytes kAccountHistoryValue3{*silkworm::from_hex(
        "0100000000000000000000003b30270000000040202b003f002c001c002d0009002e000a002f000000300000003100000032000b003300"
        "0200340011003500030036000a003700040038000800390000003a0000003b0007003c0000003d000a003e0003003f0002004000060041"
        "000200420002004300010044000200450003004700050048000400490039004a0012004b0003004c0012004d00c2004e0010004f000500"
        "50007a0051001700520000005300650049010000c901000003020000170200002d0200002f02000031020000330200004b020000510200"
        "00750200007d020000930200009d020000af020000b1020000b3020000c3020000c5020000db020000e3020000e9020000f7020000fd02"
        "000003030000070300000d03000015030000210300002b0300009f030000c5030000cd030000f3030000790500009b050000a70500009d"
        "060000c3060000c5060000988d9b8d9c8d9d8d9f8da08da18da38da48da58da68da78da88da98dab8dac8dad8dae8daf8db08db18db28d"
        "b38db48db58dba8dbb8dbc8dbd8dbe8dbf8dc18dc28dc38dc48dc58dc68dc88dc98dca8d598fa7a2f6a2f9a207a344a3c8a331a446a423"
        "ad27ad37ae3cae40ae58ee5aee61eeb8eebeee44ef91ef9cef23f189f1c403ec033c047905b605d4120d133b147d147b168616641a5624"
        "c2cec6dce5dcd7df25e02ee071e093e0a2e00ae11de344e387e3a3e3abe37de43b249824413f5741734203549654a554bc5419db204529"
        "4530454c45d4abf0ab05ac0cac13ac18ac00b9dfb63f7fe3535bc76de078e080e088e095e09ce0a7e0aee0b4e0b8e0bde014431a4306a6"
        "d625e025ed25ff252e39ed3916722972497258725f7250735f738e749b74587c9b7c7da001657983a0d5a9d5c91fcd1f1a2046216d4975"
        "4a084bef6cf376418d8f8d113f4b49a1491a4db5e9ec542355a35c816b9a6cc3719e791c8909b4ce45f817bf4c074de94dfb4d154e1a4e"
        "1e4e714fa6b183bd84bd87bd8cbd8fbdc0bdabc0b8c0dbc0ebc011c5740543065c06630666436843754341754975f6a5a7ccf7e71aec2e"
        "fab12676415dfb73f280f287f2040f21369b5818863c86a5b2b4b2bab2afb4277fdf7ff27ff97fbd80cf808da643a80db4dbe3d2ff6511"
        "69116b116d116e1171117311d813f5138f149214c8142615411544156a1575157d157e15a415a515f31777448e44ba4d3155625b685b35"
        "5c425c585c465de15dd26b4f7250726072219328935d935e93a193e493e593e693ee93ef93f893fa931b941c94f3abb4aebeaeb2af6cb5"
        "fccc29cf09004cb4000037be000039be00003bbe09005dd1000060d1000062d100007fea010068eb0000af2b2442a79900d99e367b394d"
        "5fa17448c94dc98bcbe0cdf7cd74ce7dce86ceeecefece12cf30cf36cf3ccf49cf630a9c0a2025b93608500f5023502a502b5035503d50"
        "3e5043504b504d504e5054505650d250d750dd50e450e950f250f750fe5003510d6214621a621f6225622b62336235623a624162426247"
        "624c62556262626b6271627d6282628d6294629a62a162aa62b362b962bf62c462ca62ce62d662d762df62e762ee62f762ff6201630663"
        "0d6316631b632063286331633a6343634963506355635b6361636463706375637e63866387638f6394639a63a063a663ae63b463ba63c1"
        "63c663cb63d163db63e263e963ee63f363f863d78172947b948494899496949d94a494ad94b194b894be94c394cc94d594dd94e294e794"
        "ef94f694fb94029507950d9514951a951f9526952e9538953d9549954f9558955c9561956c95749577958095859592959795a395ad95b4"
        "95ba95bb95c195c895d195d695e395e895ee95f595fb95049610961696229628962e9634963d9646964f96549605975b97bd9714a234a7"
        "50c16fc501d80ad814d82cd841d84bd863d870d87cd84de3c3e989fb93fba7fbc9fefffe54ffdb07fb3f664f9c5099587d8a418b888be4"
        "8e2a90e49d91b59ddfd7e55be61de86ef3f1096579667a0a7def8bbcbb0b0f3b16974265537753895392539b53a453ad53b653d153da53"
        "e853f053f553fe530754105422542b5434543d5446544b54505461546a5473547c5485549754a054bb54c454ce54de54e654f154fa5403"
        "55095515551e55275542555d5578559c55a555aa55b355c055c555db55ed55f655fe550356085611561a5623562c5635563e5662566b56"
        "745686568f569856a156b356bc56c556ce56f256fb5604570a571f57315743574c575e577057795781578b5794579d57a657af57c157dc"
        "57e557f457fc57095819581b5824582d583b584858515862586c5875587e589958a058ab58b458bd58aa5a635dbc5dd65d568bc79a279b"
        "09000f770300147704001e770800287700003ba1000042be0000e2be0000fbbe0000edc500001b369f2b7444cf78de78327938793f7944"
        "794c79517957795c79637968796d79727979797e79837989798e79957996799d79aa79ab79b079b679bd79c279c779cf79d479da79db79"
        "e179e679ec79f379f879017a037a067a0d7a0e7a137a187a1f7a207a287a2d7a327a387a3d7a447a4a7a567a5b7a617a687afc7a017b08"
        "7b0e7b137b197b257b377b437b497b557b5f7b6a7b6c7b6d7b8d7b9f7ba97baf7bb57bbe7bc47bc57bd57bad7db37dbe7dbf7dd17de57d"
        "f17dfb7d017e0b7e157e207e287e2b7e397e4b7e517e5f7e")};

    static silkworm::Bytes kAccountChangeSetKey1{*silkworm::from_hex("00000000004366ae")};
    static silkworm::Bytes kAccountChangeSetSubkey1{*silkworm::from_hex("8ced5ad0d8da4ec211c17355ed3dbfec4cf0e5b9")};
    static silkworm::Bytes kAccountChangeSetValue1{*silkworm::from_hex("0303038c330a01a098914888dc0516d2")};

    static silkworm::Bytes kAccountChangeSetKey2{*silkworm::from_hex("00000000004366b8")};
    static silkworm::Bytes kAccountChangeSetSubkey2{*silkworm::from_hex("5e1f0c9ddbe3cb57b80c933fab5151627d7966fa")};
    static silkworm::Bytes kAccountChangeSetValue2{*silkworm::from_hex("03010408014219564ff26a00")};

    static silkworm::Bytes kAccountChangeSetKey3{*silkworm::from_hex("000000000044589b")};
    static silkworm::Bytes kAccountChangeSetSubkey3{*silkworm::from_hex("0000000000000000000000000000000000000000")};
    static silkworm::Bytes kAccountChangeSetValue3{*silkworm::from_hex("02094165832d46fa1082db")};

    test::MockDatabaseReader db_reader;
    boost::asio::thread_pool workers{1};

    ChannelFactory channel_factory = []() {
        return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
    };
    ContextPool context_pool{1, channel_factory};
    context_pool.start();

    SECTION("Call: TO present") {
        EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, silkworm::ByteView{kZeroKey}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
                SILKRPC_LOG << "EXPECT_CALL::get "
                    << " table: " << db::table::kCanonicalHashes
                    << " key: " << silkworm::to_hex(kZeroKey)
                    << " value: " << silkworm::to_hex(kZeroHeader)
                    << "\n";
                co_return kZeroHeader;
            }));
        EXPECT_CALL(db_reader, get(db::table::kConfig, silkworm::ByteView{kConfigKey}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                SILKRPC_LOG << "EXPECT_CALL::get "
                    << " table: " << db::table::kConfig
                    << " key: " << silkworm::to_hex(kConfigKey)
                    << " value: " << silkworm::to_hex(kConfigValue)
                    << "\n";
                co_return KeyValue{kConfigKey, kConfigValue};
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey1}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                SILKRPC_LOG << "EXPECT_CALL::get "
                    << " table: " << db::table::kAccountHistory
                    << " key: " << silkworm::to_hex(kAccountHistoryKey1)
                    << " value: " << silkworm::to_hex(kAccountHistoryValue1)
                    << "\n";
                co_return KeyValue{kAccountHistoryKey1, kAccountHistoryValue1};
            }));
        EXPECT_CALL(db_reader,
                get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey1},
                                silkworm::ByteView{kAccountChangeSetSubkey1}))
            .WillOnce(InvokeWithoutArgs(
                []() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
                    SILKRPC_LOG << "EXPECT_CALL::get_both_range "
                        << " table: " << db::table::kPlainAccountChangeSet
                        << " key: " << silkworm::to_hex(kAccountChangeSetKey1)
                        << " subkey: " << silkworm::to_hex(kAccountChangeSetSubkey1)
                        << " value: " << silkworm::to_hex(kAccountChangeSetValue1)
                        << "\n";
                    co_return kAccountChangeSetValue1;
                }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey2}))
            .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                SILKRPC_LOG << "EXPECT_CALL::get "
                    << " table: " << db::table::kAccountHistory
                    << " key: " << silkworm::to_hex(kAccountHistoryKey2)
                    << " value: " << silkworm::to_hex(kAccountHistoryValue2)
                    << "\n";
                co_return KeyValue{kAccountHistoryKey2, kAccountHistoryValue2};
            }));
        EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey3}))
            .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
                SILKRPC_LOG << "EXPECT_CALL::get "
                    << " table: " << db::table::kAccountHistory
                    << " key: " << silkworm::to_hex(kAccountHistoryKey3)
                    << " value: " << silkworm::to_hex(kAccountHistoryValue3)
                    << "\n";
                co_return KeyValue{kAccountHistoryKey3, kAccountHistoryValue3};
            }));
        EXPECT_CALL(db_reader, get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey2},
                                silkworm::ByteView{kAccountChangeSetSubkey2}))
            .WillOnce(InvokeWithoutArgs(
                []() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
                    SILKRPC_LOG << "EXPECT_CALL::get_both_range "
                        << " table: " << db::table::kPlainAccountChangeSet
                        << " key: " << silkworm::to_hex(kAccountChangeSetKey2)
                        << " subkey: " << silkworm::to_hex(kAccountChangeSetSubkey2)
                        << " value: " << silkworm::to_hex(kAccountChangeSetValue2)
                        << "\n";
                    co_return kAccountChangeSetValue2;
                }));
        EXPECT_CALL(db_reader, get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey3},
                                silkworm::ByteView{kAccountChangeSetSubkey3}))
            .WillOnce(InvokeWithoutArgs(
                []() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
                    SILKRPC_LOG << "EXPECT_CALL::get_both_range "
                        << " table: " << db::table::kPlainAccountChangeSet
                        << " key: " << silkworm::to_hex(kAccountChangeSetKey3)
                        << " subkey: " << silkworm::to_hex(kAccountChangeSetSubkey3)
                        << " value: " << silkworm::to_hex(kAccountChangeSetValue3)
                        << "\n";
                    co_return kAccountChangeSetValue3;
                }));

        const auto block_number = 4'417'196;  // 0x4366AC
        silkrpc::Call call;
        call.from = 0x8ced5ad0d8da4ec211c17355ed3dbfec4cf0e5b9_address;
        call.to = 0x5e1f0c9ddbe3cb57b80c933fab5151627d7966fa_address;
        call.value = 50'000'000;
        call.gas = 30'000;
        call.gas_price = 1'000'000'000;
        call.data = *silkworm::from_hex("00");

        silkworm::Block block{};
        block.header.number = block_number;

        DebugExecutor executor{context_pool.next_io_context(), db_reader, workers};
        boost::asio::io_context& io_context = context_pool.next_io_context();
        auto execution_result = boost::asio::co_spawn(io_context.get_executor(), executor.execute(block, call), boost::asio::use_future);
        auto result = execution_result.get();

        context_pool.stop();
        context_pool.join();

        CHECK(result.pre_check_error.has_value() == false);
        CHECK(result.debug_trace == R"({
            "failed": false,
            "gas": 21004,
            "returnValue": "",
            "structLogs": []
        })"_json);
    }
}

TEST_CASE("DebugExecutor::execute call with error") {
    SILKRPC_LOG_STREAMS(null_stream(), null_stream());
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    static silkworm::Bytes kAccountHistoryKey1{*silkworm::from_hex("578f0a154b23be77fc2033197fbc775637648ad400000000005279a8")};
    static silkworm::Bytes kAccountHistoryValue1{*silkworm::from_hex(
        "0100000000000000000000003a300000010000005200650010000000a074f7740275247527752b75307549756d75787581758a75937598"
        "759e75a275a775a975af75b775ce75eb75f67508760f7613769f76a176a376be76ca76ce76d876e576fb76267741776177627769777077"
        "7c7783779e79a279a579a779ad79b279448a458a4d8a568a638acfa0d4a0d6a0d8a0dfa0e2a0e5a0e9a0eca0efa0f1a0f5a0fea003a10a"
        "a117a11ba131a135a139a152a154a158a15aa15da171a175a17ca1b4a1e4a124a21fbb22bb26bb2bbb2dbb2fbb34bb38bb3fbb0bd14ed2"
        "a0e5a3e550eb60eb6ceb72eb")};

    static silkworm::Bytes kAccountHistoryKey2{*silkworm::from_hex("6951c35e335fa18c97cb207119133cd8009580cd00000000005279a8")};
    static silkworm::Bytes kAccountHistoryValue2{*silkworm::from_hex(
        "0100000000000000000000003a3000000700000044000a004600010048000100490005004c0001004d0001005e"
        "00000040000000560000005a0000005e0000006a0000006e000000720000005da562a563a565a567a56aa59da5"
        "a0a5f0a5f5a57ef926a863a8eb520b535d1b951bb71b3c1c741caa4f53f5b0f5184f536018f6")};

    static silkworm::Bytes kAccountHistoryKey3{*silkworm::from_hex("000000000000000000000000000000000000000000000000005279a8")};
    static silkworm::Bytes kAccountHistoryValue3{*silkworm::from_hex(
        "0100000000000000000000003b30270000000040202b003f002c001c002d0009002e000a002f000000300000003100000032000b003300"
        "0200340011003500030036000a003700040038000800390000003a0000003b0007003c0000003d000a003e0003003f0002004000060041"
        "000200420002004300010044000200450003004700050048000400490039004a0012004b0003004c0012004d00c2004e0010004f000500"
        "50007a0051001700520000005300650049010000c901000003020000170200002d0200002f02000031020000330200004b020000510200"
        "00750200007d020000930200009d020000af020000b1020000b3020000c3020000c5020000db020000e3020000e9020000f7020000fd02"
        "000003030000070300000d03000015030000210300002b0300009f030000c5030000cd030000f3030000790500009b050000a70500009d"
        "060000c3060000c5060000988d9b8d9c8d9d8d9f8da08da18da38da48da58da68da78da88da98dab8dac8dad8dae8daf8db08db18db28d"
        "b38db48db58dba8dbb8dbc8dbd8dbe8dbf8dc18dc28dc38dc48dc58dc68dc88dc98dca8d598fa7a2f6a2f9a207a344a3c8a331a446a423"
        "ad27ad37ae3cae40ae58ee5aee61eeb8eebeee44ef91ef9cef23f189f1c403ec033c047905b605d4120d133b147d147b168616641a5624"
        "c2cec6dce5dcd7df25e02ee071e093e0a2e00ae11de344e387e3a3e3abe37de43b249824413f5741734203549654a554bc5419db204529"
        "4530454c45d4abf0ab05ac0cac13ac18ac00b9dfb63f7fe3535bc76de078e080e088e095e09ce0a7e0aee0b4e0b8e0bde014431a4306a6"
        "d625e025ed25ff252e39ed3916722972497258725f7250735f738e749b74587c9b7c7da001657983a0d5a9d5c91fcd1f1a2046216d4975"
        "4a084bef6cf376418d8f8d113f4b49a1491a4db5e9ec542355a35c816b9a6cc3719e791c8909b4ce45f817bf4c074de94dfb4d154e1a4e"
        "1e4e714fa6b183bd84bd87bd8cbd8fbdc0bdabc0b8c0dbc0ebc011c5740543065c06630666436843754341754975f6a5a7ccf7e71aec2e"
        "fab12676415dfb73f280f287f2040f21369b5818863c86a5b2b4b2bab2afb4277fdf7ff27ff97fbd80cf808da643a80db4dbe3d2ff6511"
        "69116b116d116e1171117311d813f5138f149214c8142615411544156a1575157d157e15a415a515f31777448e44ba4d3155625b685b35"
        "5c425c585c465de15dd26b4f7250726072219328935d935e93a193e493e593e693ee93ef93f893fa931b941c94f3abb4aebeaeb2af6cb5"
        "fccc29cf09004cb4000037be000039be00003bbe09005dd1000060d1000062d100007fea010068eb0000af2b2442a79900d99e367b394d"
        "5fa17448c94dc98bcbe0cdf7cd74ce7dce86ceeecefece12cf30cf36cf3ccf49cf630a9c0a2025b93608500f5023502a502b5035503d50"
        "3e5043504b504d504e5054505650d250d750dd50e450e950f250f750fe5003510d6214621a621f6225622b62336235623a624162426247"
        "624c62556262626b6271627d6282628d6294629a62a162aa62b362b962bf62c462ca62ce62d662d762df62e762ee62f762ff6201630663"
        "0d6316631b632063286331633a6343634963506355635b6361636463706375637e63866387638f6394639a63a063a663ae63b463ba63c1"
        "63c663cb63d163db63e263e963ee63f363f863d78172947b948494899496949d94a494ad94b194b894be94c394cc94d594dd94e294e794"
        "ef94f694fb94029507950d9514951a951f9526952e9538953d9549954f9558955c9561956c95749577958095859592959795a395ad95b4"
        "95ba95bb95c195c895d195d695e395e895ee95f595fb95049610961696229628962e9634963d9646964f96549605975b97bd9714a234a7"
        "50c16fc501d80ad814d82cd841d84bd863d870d87cd84de3c3e989fb93fba7fbc9fefffe54ffdb07fb3f664f9c5099587d8a418b888be4"
        "8e2a90e49d91b59ddfd7e55be61de86ef3f1096579667a0a7def8bbcbb0b0f3b16974265537753895392539b53a453ad53b653d153da53"
        "e853f053f553fe530754105422542b5434543d5446544b54505461546a5473547c5485549754a054bb54c454ce54de54e654f154fa5403"
        "55095515551e55275542555d5578559c55a555aa55b355c055c555db55ed55f655fe550356085611561a5623562c5635563e5662566b56"
        "745686568f569856a156b356bc56c556ce56f256fb5604570a571f57315743574c575e577057795781578b5794579d57a657af57c157dc"
        "57e557f457fc57095819581b5824582d583b584858515862586c5875587e589958a058ab58b458bd58aa5a635dbc5dd65d568bc79a279b"
        "09000f770300147704001e770800287700003ba1000042be0000e2be0000fbbe0000edc500001b369f2b7444cf78de78327938793f7944"
        "794c79517957795c79637968796d79727979797e79837989798e79957996799d79aa79ab79b079b679bd79c279c779cf79d479da79db79"
        "e179e679ec79f379f879017a037a067a0d7a0e7a137a187a1f7a207a287a2d7a327a387a3d7a447a4a7a567a5b7a617a687afc7a017b08"
        "7b0e7b137b197b257b377b437b497b557b5f7b6a7b6c7b6d7b8d7b9f7ba97baf7bb57bbe7bc47bc57bd57bad7db37dbe7dbf7dd17de57d"
        "f17dfb7d017e0b7e157e207e287e2b7e397e4b7e517e5f7e")};

    static silkworm::Bytes kPlainStateKey{*silkworm::from_hex("6951c35e335fa18c97cb207119133cd8009580cd")};

    static silkworm::Bytes kAccountChangeSetKey{*silkworm::from_hex("00000000005279ad")};
    static silkworm::Bytes kAccountChangeSetSubkey{*silkworm::from_hex("578f0a154b23be77fc2033197fbc775637648ad4")};
    static silkworm::Bytes kAccountChangeSetValue{*silkworm::from_hex("03012f090207fbc719f215d705")};

    static silkworm::Bytes kAccountChangeSetKey1{*silkworm::from_hex("00000000005EF618")};
    static silkworm::Bytes kAccountChangeSetSubkey1{*silkworm::from_hex("6951c35e335fa18c97cb207119133cd8009580cd")};
    static silkworm::Bytes kAccountChangeSetValue1{*silkworm::from_hex("00000000005279a8")};

    static silkworm::Bytes kAccountChangeSetKey2{*silkworm::from_hex("0000000000532b9f")};
    static silkworm::Bytes kAccountChangeSetSubkey2{*silkworm::from_hex("0000000000000000000000000000000000000000")};
    static silkworm::Bytes kAccountChangeSetValue2{*silkworm::from_hex("020944ed67f28fd50bb8e9")};

    test::MockDatabaseReader db_reader;
    boost::asio::thread_pool workers{1};

    ChannelFactory channel_factory = []() {
        return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
    };
    ContextPool context_pool{1, channel_factory};
    context_pool.start();

    EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, silkworm::ByteView{kZeroKey}))
        .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
            SILKRPC_LOG << "EXPECT_CALL::get_one "
                << " table: " << db::table::kCanonicalHashes
                << " key: " << silkworm::to_hex(kZeroKey)
                << " value: " << silkworm::to_hex(kZeroHeader)
                << "\n";
            co_return kZeroHeader;
        }));
    EXPECT_CALL(db_reader, get(db::table::kConfig, silkworm::ByteView{kConfigKey}))
        .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
            SILKRPC_LOG << "EXPECT_CALL::get "
                << " table: " << db::table::kConfig
                << " key: " << silkworm::to_hex(kConfigKey)
                << " value: " << silkworm::to_hex(kConfigValue)
                << "\n";
            co_return KeyValue{kConfigKey, kConfigValue};
        }));
    EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey1}))
        .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
            SILKRPC_LOG << "EXPECT_CALL::get "
                << " table: " << db::table::kAccountHistory
                << " key: " << silkworm::to_hex(kAccountHistoryKey1)
                << " value: " << silkworm::to_hex(kAccountHistoryValue1)
                << "\n";
            co_return KeyValue{kAccountHistoryKey1, kAccountHistoryValue1};
        }));
    EXPECT_CALL(db_reader,
            get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey},
                            silkworm::ByteView{kAccountChangeSetSubkey}))
        .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
            SILKRPC_LOG << "EXPECT_CALL::get_both_range "
                << " table: " << db::table::kPlainAccountChangeSet
                << " key: " << silkworm::to_hex(kAccountChangeSetKey)
                << " subkey: " << silkworm::to_hex(kAccountChangeSetSubkey)
                << " value: " << silkworm::to_hex(kAccountChangeSetValue)
                << "\n";
            co_return kAccountChangeSetValue;
        }));
    EXPECT_CALL(db_reader,
            get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey1},
                            silkworm::ByteView{kAccountChangeSetSubkey1}))
        .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
            SILKRPC_LOG << "NON DOVREBBE SUCCEDERE EXPECT_CALL::get_both_range "
                << " table: " << db::table::kPlainAccountChangeSet
                << " key: " << silkworm::to_hex(kAccountChangeSetKey1)
                << " subkey: " << silkworm::to_hex(kAccountChangeSetSubkey1)
                << " value: " << silkworm::to_hex(kAccountChangeSetValue1)
                << "\n";
            co_return kAccountChangeSetValue1;
        }));
    EXPECT_CALL(db_reader,
            get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey2},
                            silkworm::ByteView{kAccountChangeSetSubkey2}))
        .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
            SILKRPC_LOG << "NON DOVREBBE SUCCEDERE EXPECT_CALL::get_both_range "
                << " table: " << db::table::kPlainAccountChangeSet
                << " key: " << silkworm::to_hex(kAccountChangeSetKey2)
                << " subkey: " << silkworm::to_hex(kAccountChangeSetSubkey2)
                << " value: " << silkworm::to_hex(kAccountChangeSetValue2)
                << "\n";
            co_return kAccountChangeSetValue2;
        }));
    EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey2}))
        .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
            SILKRPC_LOG << "EXPECT_CALL::get "
                << " table: " << db::table::kAccountHistory
                << " key: " << silkworm::to_hex(kAccountHistoryKey2)
                << " value: " << silkworm::to_hex(kAccountHistoryValue2)
                << "\n";
            co_return KeyValue{kAccountHistoryKey2, kAccountHistoryValue2};
        }));
    EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey3}))
        .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
            SILKRPC_LOG << "EXPECT_CALL::get "
                << " table: " << db::table::kAccountHistory
                << " key: " << silkworm::to_hex(kAccountHistoryKey3)
                << " value: " << silkworm::to_hex(kAccountHistoryValue3)
                << "\n";
            co_return KeyValue{kAccountHistoryKey3, kAccountHistoryValue3};
        }));
    EXPECT_CALL(db_reader, get_one(db::table::kPlainState, silkworm::ByteView{kPlainStateKey}))
        .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
            SILKRPC_LOG << "EXPECT_CALL::get "
                << " table: " << db::table::kPlainState
                << " key: " << silkworm::to_hex(kPlainStateKey)
                << " value: "
                << "\n";
            co_return silkworm::Bytes{};
        }));

    uint64_t block_number = 5'405'095;  // 0x5279A7

    silkrpc::Call call;
    call.from = 0x578f0a154b23be77fc2033197fbc775637648ad4_address;
    call.value = 0;
    call.gas = 211'190;
    call.gas_price = 14;
    call.data = *silkworm::from_hex(
        "0x414bf3890000000000000000000000009d381f0b1637475f133c92d9b9fdc5493ae19b630000000000000000000000009b73fc19"
        "3bfa16abe18d1ea30734e4a6444a753f00000000000000000000000000000000000000000000000000000000000027100000000000"
        "00000000000000578f0a154b23be77fc2033197fbc775637648ad40000000000000000000000000000000000000000000000000000"
        "0000612ba19c00000000000000000000000000000000000000000001a784379d99db42000000000000000000000000000000000000"
        "00000000000002cdc48e6cca575707722c0000000000000000000000000000000000000000000000000000000000000000");

    silkworm::Block block{};
    block.header.number = block_number;

    DebugExecutor executor{context_pool.next_io_context(), db_reader, workers};
    boost::asio::io_context& io_context = context_pool.next_io_context();
    auto execution_result = boost::asio::co_spawn(io_context.get_executor(), executor.execute(block, call), boost::asio::use_future);
    auto result = execution_result.get();

    context_pool.stop();
    context_pool.join();

    CHECK(result.pre_check_error.has_value() == false);
    CHECK(result.debug_trace == R"({
        "failed": true,
        "gas": 211190,
        "returnValue": "",
        "structLogs": [
            {
                "depth": 1,
                "gas": 156082,
                "gasCost": 2,
                "memory": [],
                "op": "COINBASE",
                "pc": 0,
                "stack": []
            },
            {
                "depth": 1,
                "error": {},
                "gas": 156080,
                "gasCost": 2,
                "memory": [],
                "op": "opcode 0x4b not defined",
                "pc": 1,
                "stack": [
                    "0x0"
                ]
            }
        ]
    })"_json);
}

TEST_CASE("DebugExecutor::execute block") {
    SILKRPC_LOG_STREAMS(null_stream(), null_stream());
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    static silkworm::Bytes kAccountHistoryKey1{*silkworm::from_hex("daae090d53f9ed9e2e1fd25258c01bac4dd6d1c500000000000fa0a5")};
    static silkworm::Bytes kAccountHistoryValue1{*silkworm::from_hex(
        "0100000000000000000000003a300000020000000e0004000f0031001800000022000000eca7f4a7d3a9dea9dfa9fd1b191c301cb91cbe"
        "1cf21cfc1c0f1d141d261d801d911da61d00440e4a485f4f5f427b537baf7bb17bb57bb97bbf7bc57bc97bd87bda7be17be47be97bfa7b"
        "fe7b017c267c297c2c7c367c3a9d3b9d3d9d429d47a071a0a5a0aea0b4a0b8a0c3a0c9a0")};

    static silkworm::Bytes kAccountHistoryKey2{*silkworm::from_hex("a85b4c37cd8f447848d49851a1bb06d10d410c1300000000000fa0a5")};
    static silkworm::Bytes kAccountHistoryValue2{*silkworm::from_hex("0100000000000000000000003a300000010000000f00000010000000a5a0")};

    static silkworm::Bytes kAccountHistoryKey3{*silkworm::from_hex("000000000000000000000000000000000000000000000000000fa0a5")};
    static silkworm::Bytes kAccountHistoryValue3{*silkworm::from_hex(
        "0100000000000000000000003b301800000001000000000002000100040007000600030008000200090000000e0011000f00060011000f"
        "00130003001a0000001c0003001d0000001e0000001f00370020001d002100270222006b00230019002400320025004d00260004002700"
        "04002a000f002b002700d0000000d2000000d6000000e6000000ee000000f4000000f60000001a01000028010000480100005001000052"
        "0100005a0100005c0100005e010000ce0100000a02000000050000d80500000c060000720600000e070000180700002207000042070000"
        "0000d03cd13cd1b6d3b617b718b719b72ab72cb774fa4611c695c795c8957184728474842d12377d4c7d547d767e848053819c81dc81d9"
        "8fee8f059022902f9035903c903f904a9091902eb0fee1ffe101e202e203e205e2e6b1e8b1e9b1eab1edb1eeb1f0b1f1b1f2b1f3b1f5b1"
        "f6b1f7b1f9b1fab1fcb1de62e562e662f2625209b453ba53c153d65304ebb1007f4b8a4b314c9b4c685dc25dcc5df05d045e0c5e315e51"
        "5eb55e0f5f105f2d5fac890f9031907f907e9f0ca0f1a0f6a0faa009a120a126a1f3a1f5a1b1a2b3a21ca41fa425a445a456a458a443a5"
        "95a698a68ad190d1a1e249e577e570e6c3e936f940f921fe28fe2dfe27ff39ff83ff25123612371230439f434d598c593d6c676c996ca0"
        "6cc16cf26c337114826183e386f59729983b9870f284f2a2f283f3a1f3b7f3faf702f84cfa53fabd00d4070000d8070000dd0700000c08"
        "0000730800007f080000c20c00003b1e00003f1e0000671e00006a1e0000ea200000fd200100f8230000ac240000333600008d3600009d"
        "370000673a00000c3b00000b520000105200004d540200c2690000ce690100eb690100ee690000176a0400f9770000d4780000de780000"
        "e478000076790000de790100e1790200007a0100037a0200297a04005b7c0a00677c04006d7c00006f7c0600777c0000797c0600817c00"
        "00837c06008b7c00008d7c0600957c0000977c06009f7c0400a57c0200a97c0000ab7c0500b37c0700bd7c0000bf7c0400c57c0300eb7c"
        "0000f97c0100017d0000057d00000d7d00001c7d0300217d08002b7d00002d7d0600357d0000377d06003f7d0000417d0500497d070053"
        "7d0400597d02005d7d0000607d0500677d0000697d0800737d08007e7d0500857d0000877d0300ba7d0000bd7d0000cc7d0000d47d0000"
        "118e0000978e0000aa8e0000128f0300178f0000198f0700238f0300288f0100408f0100438f06004b8f0400518f08005b8f01005e8f08"
        "00698f00006b8f01006e8f0300748f07007d8f0000808f0300858f0000878f03008c8f0500948f020024900100279001002a9000002c90"
        "020031900000349002003a9002003f9001004290000045900300759000001c91000013a8000023a8000043a8000055aa0000adab0100ca"
        "bd0000b9c20000d9c20000e2c20000f8c2000031d100004ed1000051d1000062d1040068d1070071d109007cd1050084d105008bd10800"
        "95d1000097d106009fd10000a1d10100a4d10300abd10100aed10300b3d10000b5d10000b7d10400bdd10000bfd10200c3d10200c7d100"
        "00cad10200ced10600f6d100007bd20000afd2000038d402006cd4000086d402008ad401008dd400008fd40100c6d5000099d60600a1d6"
        "0400a7d60000a9d60000acd60000aed60100c7d60000d4d60500dbd60200f2d60100f5d60200fad6020010d7010013d7030019d700001b"
        "d701001ed7050025d704002bd7080035d70600a1d80000bad80000701777178b1793179b17ca17db1708181a1829183a183c183d183f18"
        "7a1a811a941a9b1a2f1b371b3a1b514451475d4763477047f147f84701480748114818481c482f483d4843484b48ec59d45a6c5b0f5dca"
        "716f72707271721ba320a37fa585a5c6b6f9b6fbb604b752b899b8b8b8e6b83eb98fb990b991b9bfbac7ba33ca47ca8ecb93cb58cc5fcc"
        "f7cd6ed3c9d6ccd6d5d6a5e4b5e4d6e46fe58be596e597e598e599e59be59ce59ee59fe5a0e5a1e5aae5ace5b4e5b5e5b6e5bbe5bce5bd"
        "e5c0e5c7e5c8e5ece5ede5eee5fae6ffe65cf6e3f7b4f9160e89108a109310aa100d118412ad5681669a669c66f86646675d679f67e067"
        "1c68d86aa26dba6dba81c881b0820298219a40edb809cb09d909b60ad10ac00b3b8f618f958fbc90fba420a53ba5d5baedba07bb40bbb2"
        "bbe2bb02bcd0bef0bf8bc08ec02ace40ce41ce38cfd8d181d4a1d4a3d4dce45be55ee567e572e578e590e59be5a1e5c0e5b8e6dbe693e8"
        "9ee8fbe925ea53eaf6ecd7eea02ab42ae82afa2a042b222bb33db43db63dd13dd23dd53dd83dd93dda3dff3e003f2b3f2c3f2e3f423f43"
        "3f443f4d3f4e3f1c4034402841b741d641e34114424f422d447944a444a944c444c844bd5537563e5644564f567a565b572458a669dd6b"
        "1071127129716c719c71d171ed7115725d74a982ad82ce82d182d68277c47dc40bc53ac767c78cc7bcc71cc823c828c82dc892caa2caa3"
        "cbbdcb39783e8391b992b93dffbb05c205728f928fb6c7b44a365b3f5b08b1f2c41bc52bc57dc592cafbca39cd79cd96f15af221f338f3"
        "c434a94baa4ba84d424e1252125af45e625f645f6e5f556357637a633e64cf64fb66fc66fd66fe66ff6601670267036704670567066708"
        "6709670a67a575f87a4b7b537b157dec7f938d948d958d968d")};

    static silkworm::Bytes kAccountChangeSetKey{*silkworm::from_hex("00000000000fa0a5")};
    static silkworm::Bytes kAccountChangeSetSubkey1{*silkworm::from_hex("daae090d53f9ed9e2e1fd25258c01bac4dd6d1c5")};
    static silkworm::Bytes kAccountChangeSetValue1{*silkworm::from_hex("030127080334e1d62a9e3440")};

    static silkworm::Bytes kAccountChangeSetSubkey2{*silkworm::from_hex("a85b4c37cd8f447848d49851a1bb06d10d410c13")};
    static silkworm::Bytes kAccountChangeSetValue2{*silkworm::from_hex("")};

    static silkworm::Bytes kAccountChangeSetKey3{*silkworm::from_hex("00000000000fb02e")};
    static silkworm::Bytes kAccountChangeSetSubkey3{*silkworm::from_hex("0000000000000000000000000000000000000000")};
    static silkworm::Bytes kAccountChangeSetValue3{*silkworm::from_hex("0208028ded68c33d1401")};

    test::MockDatabaseReader db_reader;
    boost::asio::thread_pool workers{1};

    ChannelFactory channel_factory = []() {
        return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials());
    };
    ContextPool context_pool{1, channel_factory};
    context_pool.start();

    EXPECT_CALL(db_reader, get_one(db::table::kCanonicalHashes, silkworm::ByteView{kZeroKey}))
        .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<silkworm::Bytes> {
            SILKRPC_LOG << "EXPECT_CALL::get_one "
                << " table: " << db::table::kCanonicalHashes
                << " key: " << silkworm::to_hex(kZeroKey)
                << " value: " << silkworm::to_hex(kZeroHeader)
                << "\n";
            co_return kZeroHeader;
        }));
    EXPECT_CALL(db_reader, get(db::table::kConfig, silkworm::ByteView{kConfigKey}))
        .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
            SILKRPC_LOG << "EXPECT_CALL::get "
                << " table: " << db::table::kConfig
                << " key: " << silkworm::to_hex(kConfigKey)
                << " value: " << silkworm::to_hex(kConfigValue)
                << "\n";
            co_return KeyValue{kConfigKey, kConfigValue};
        }));
    EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey1}))
        .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
            SILKRPC_LOG << "EXPECT_CALL::get "
                << " table: " << db::table::kAccountHistory
                << " key: " << silkworm::to_hex(kAccountHistoryKey1)
                << " value: " << silkworm::to_hex(kAccountHistoryValue1)
                << "\n";
            co_return KeyValue{kAccountHistoryKey1, kAccountHistoryValue1};
        }));
    EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey2}))
        .WillRepeatedly(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
            SILKRPC_LOG << "EXPECT_CALL::get "
                << " table: " << db::table::kAccountHistory
                << " key: " << silkworm::to_hex(kAccountHistoryKey2)
                << " value: " << silkworm::to_hex(kAccountHistoryValue2)
                << "\n";
            co_return KeyValue{kAccountHistoryKey2, kAccountHistoryValue2};
        }));
    EXPECT_CALL(db_reader, get(db::table::kAccountHistory, silkworm::ByteView{kAccountHistoryKey3}))
        .WillOnce(InvokeWithoutArgs([]() -> boost::asio::awaitable<KeyValue> {
            SILKRPC_LOG << "EXPECT_CALL::get "
                << " table: " << db::table::kAccountHistory
                << " key: " << silkworm::to_hex(kAccountHistoryKey3)
                << " value: " << silkworm::to_hex(kAccountHistoryValue3)
                << "\n";
            co_return KeyValue{kAccountHistoryKey3, kAccountHistoryValue3};
        }));
    EXPECT_CALL(db_reader,
            get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey},
                            silkworm::ByteView{kAccountChangeSetSubkey1}))
        .WillOnce(InvokeWithoutArgs(
            []() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
            SILKRPC_LOG << "EXPECT_CALL::get_both_range "
                << " table: " << db::table::kPlainAccountChangeSet
                << " key: " << silkworm::to_hex(kAccountChangeSetKey)
                << " subkey: " << silkworm::to_hex(kAccountChangeSetSubkey1)
                << " value: " << silkworm::to_hex(kAccountChangeSetValue1)
                << "\n";
                co_return kAccountChangeSetValue1;
            }));
    EXPECT_CALL(db_reader,
            get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey},
                            silkworm::ByteView{kAccountChangeSetSubkey2}))
        .WillRepeatedly(InvokeWithoutArgs(
            []() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
            SILKRPC_LOG << "EXPECT_CALL::get_both_range "
                << " table: " << db::table::kPlainAccountChangeSet
                << " key: " << silkworm::to_hex(kAccountChangeSetKey)
                << " subkey: " << silkworm::to_hex(kAccountChangeSetSubkey2)
                << " value: " << silkworm::to_hex(kAccountChangeSetValue2)
                << "\n";
                co_return kAccountChangeSetValue2;
            }));
    EXPECT_CALL(db_reader,
            get_both_range(db::table::kPlainAccountChangeSet, silkworm::ByteView{kAccountChangeSetKey3},
                            silkworm::ByteView{kAccountChangeSetSubkey3}))
        .WillOnce(InvokeWithoutArgs(
            []() -> boost::asio::awaitable<std::optional<silkworm::Bytes>> {
            SILKRPC_LOG << "EXPECT_CALL::get_both_range "
                << " table: " << db::table::kPlainAccountChangeSet
                << " key: " << silkworm::to_hex(kAccountChangeSetKey3)
                << " subkey: " << silkworm::to_hex(kAccountChangeSetSubkey3)
                << " value: " << silkworm::to_hex(kAccountChangeSetValue3)
                << "\n";
                co_return kAccountChangeSetValue3;
            }));

    uint64_t block_number = 1'024'165;

    silkworm::Block block{};
    block.header.number = block_number;

    block.transactions.resize(1);
    auto& transaction = block.transactions.at(0);
    transaction.from = 0xdaae090d53f9ed9e2e1fd25258c01bac4dd6d1c5_address;
    transaction.gas_limit = 4700000;
    transaction.max_fee_per_gas = 1'000'000'000;
    transaction.max_priority_fee_per_gas = 1'000'000'000;
    transaction.data = *silkworm::from_hex(
        "60806040526000805534801561001457600080fd5b5060c6806100236000396000f3fe6080604052348015600f57600080fd5b50600436"
        "1060325760003560e01c806360fe47b11460375780636d4ce63c146062575b600080fd5b606060048036036020811015604b57600080fd"
        "5b8101908080359060200190929190505050607e565b005b60686088565b6040518082815260200191505060405180910390f35b806000"
        "8190555050565b6000805490509056fea265627a7a72305820ca7603d2458ae7a9db8bde091d8ba88a4637b54a8cc213b73af865f97c60"
        "af2c64736f6c634300050a0032");

    DebugExecutor executor{context_pool.next_io_context(), db_reader, workers};

    boost::asio::io_context& io_context = context_pool.next_io_context();
    auto execution_result = boost::asio::co_spawn(io_context.get_executor(), executor.execute(block), boost::asio::use_future);
    auto result = execution_result.get();

    context_pool.stop();
    context_pool.join();

    CHECK(result == R"([
        {
            "failed": false,
            "gas": 112583,
            "returnValue": "6080604052348015600f57600080fd5b506004361060325760003560e01c806360fe47b11460375780636d4ce63c146062575b600080fd5b606060048036036020811015604b57600080fd5b8101908080359060200190929190505050607e565b005b60686088565b6040518082815260200191505060405180910390f35b8060008190555050565b6000805490509056fea265627a7a72305820ca7603d2458ae7a9db8bde091d8ba88a4637b54a8cc213b73af865f97c60af2c64736f6c634300050a0032",
            "structLogs": [
                {
                    "depth": 1,
                    "gas": 4632116,
                    "gasCost": 3,
                    "memory": [],
                    "op": "PUSH1",
                    "pc": 0,
                    "stack": []
                },
                {
                    "depth": 1,
                    "gas": 4632113,
                    "gasCost": 3,
                    "memory": [],
                    "op": "PUSH1",
                    "pc": 2,
                    "stack": [
                        "0x80"
                    ]
                },
                {
                    "depth": 1,
                    "gas": 4632110,
                    "gasCost": 12,
                    "memory": [
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000"
                    ],
                    "op": "MSTORE",
                    "pc": 4,
                    "stack": [
                        "0x80",
                        "0x40"
                    ]
                },
                {
                    "depth": 1,
                    "gas": 4632098,
                    "gasCost": 3,
                    "memory": [
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000080"
                    ],
                    "op": "PUSH1",
                    "pc": 5,
                    "stack": []
                },
                {
                    "depth": 1,
                    "gas": 4632095,
                    "gasCost": 3,
                    "memory": [
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000080"
                    ],
                    "op": "DUP1",
                    "pc": 7,
                    "stack": [
                        "0x0"
                    ]
                },
                {
                    "depth": 1,
                    "gas": 4632092,
                    "gasCost": 5000,
                    "memory": [
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000080"
                    ],
                    "op": "SSTORE",
                    "pc": 8,
                    "stack": [
                        "0x0",
                        "0x0"
                    ],
                    "storage": {
                        "0000000000000000000000000000000000000000000000000000000000000000": "0000000000000000000000000000000000000000000000000000000000000000"
                    }
                },
                {
                    "depth": 1,
                    "gas": 4627092,
                    "gasCost": 2,
                    "memory": [
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000080"
                    ],
                    "op": "CALLVALUE",
                    "pc": 9,
                    "stack": []
                },
                {
                    "depth": 1,
                    "gas": 4627090,
                    "gasCost": 3,
                    "memory": [
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000080"
                    ],
                    "op": "DUP1",
                    "pc": 10,
                    "stack": [
                        "0x0"
                    ]
                },
                {
                    "depth": 1,
                    "gas": 4627087,
                    "gasCost": 3,
                    "memory": [
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000080"
                    ],
                    "op": "ISZERO",
                    "pc": 11,
                    "stack": [
                        "0x0",
                        "0x0"
                    ]
                },
                {
                    "depth": 1,
                    "gas": 4627084,
                    "gasCost": 3,
                    "memory": [
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000080"
                    ],
                    "op": "PUSH2",
                    "pc": 12,
                    "stack": [
                        "0x0",
                        "0x1"
                    ]
                },
                {
                    "depth": 1,
                    "gas": 4627081,
                    "gasCost": 10,
                    "memory": [
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000080"
                    ],
                    "op": "JUMPI",
                    "pc": 15,
                    "stack": [
                        "0x0",
                        "0x1",
                        "0x14"
                    ]
                },
                {
                    "depth": 1,
                    "gas": 4627071,
                    "gasCost": 1,
                    "memory": [
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000080"
                    ],
                    "op": "JUMPDEST",
                    "pc": 20,
                    "stack": [
                        "0x0"
                    ]
                },
                {
                    "depth": 1,
                    "gas": 4627070,
                    "gasCost": 2,
                    "memory": [
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000080"
                    ],
                    "op": "POP",
                    "pc": 21,
                    "stack": [
                        "0x0"
                    ]
                },
                {
                    "depth": 1,
                    "gas": 4627068,
                    "gasCost": 3,
                    "memory": [
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000080"
                    ],
                    "op": "PUSH1",
                    "pc": 22,
                    "stack": []
                },
                {
                    "depth": 1,
                    "gas": 4627065,
                    "gasCost": 3,
                    "memory": [
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000080"
                    ],
                    "op": "DUP1",
                    "pc": 24,
                    "stack": [
                        "0xc6"
                    ]
                },
                {
                    "depth": 1,
                    "gas": 4627062,
                    "gasCost": 3,
                    "memory": [
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000080"
                    ],
                    "op": "PUSH2",
                    "pc": 25,
                    "stack": [
                        "0xc6",
                        "0xc6"
                    ]
                },
                {
                    "depth": 1,
                    "gas": 4627059,
                    "gasCost": 3,
                    "memory": [
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000080"
                    ],
                    "op": "PUSH1",
                    "pc": 28,
                    "stack": [
                        "0xc6",
                        "0xc6",
                        "0x23"
                    ]
                },
                {
                    "depth": 1,
                    "gas": 4627056,
                    "gasCost": 36,
                    "memory": [
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000080",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000",
                        "0000000000000000000000000000000000000000000000000000000000000000"
                    ],
                    "op": "CODECOPY",
                    "pc": 30,
                    "stack": [
                        "0xc6",
                        "0xc6",
                        "0x23",
                        "0x0"
                    ]
                },
                {
                    "depth": 1,
                    "gas": 4627020,
                    "gasCost": 3,
                    "memory": [
                        "6080604052348015600f57600080fd5b506004361060325760003560e01c8063",
                        "60fe47b11460375780636d4ce63c146062575b600080fd5b6060600480360360",
                        "20811015604b57600080fd5b8101908080359060200190929190505050607e56",
                        "5b005b60686088565b6040518082815260200191505060405180910390f35b80",
                        "60008190555050565b6000805490509056fea265627a7a72305820ca7603d245",
                        "8ae7a9db8bde091d8ba88a4637b54a8cc213b73af865f97c60af2c64736f6c63",
                        "4300050a00320000000000000000000000000000000000000000000000000000"
                    ],
                    "op": "PUSH1",
                    "pc": 31,
                    "stack": [
                        "0xc6"
                    ]
                },
                {
                    "depth": 1,
                    "gas": 4627017,
                    "gasCost": 0,
                    "memory": [
                        "6080604052348015600f57600080fd5b506004361060325760003560e01c8063",
                        "60fe47b11460375780636d4ce63c146062575b600080fd5b6060600480360360",
                        "20811015604b57600080fd5b8101908080359060200190929190505050607e56",
                        "5b005b60686088565b6040518082815260200191505060405180910390f35b80",
                        "60008190555050565b6000805490509056fea265627a7a72305820ca7603d245",
                        "8ae7a9db8bde091d8ba88a4637b54a8cc213b73af865f97c60af2c64736f6c63",
                        "4300050a00320000000000000000000000000000000000000000000000000000"
                    ],
                    "op": "RETURN",
                    "pc": 33,
                    "stack": [
                        "0xc6",
                        "0x0"
                    ]
                }
            ]
        }
    ])"_json);
}


TEST_CASE("DebugTrace json serialization") {
    SILKRPC_LOG_STREAMS(null_stream(), null_stream());
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    DebugLog log;
    log.pc = 1;
    log.op = "PUSH1";
    log.gas = 3;
    log.gas_cost = 4;
    log.depth = 1;
    log.error = false;
    log.memory.push_back("0000000000000000000000000000000000000000000000000000000000000080");
    log.stack.push_back("0x80");
    log.storage["804292fe56769f4b9f0e91cf85875f67487cd9e85a084cbba2188be4466c4f23"] = "0000000000000000000000000000000000000000000000000000000000000008";

    SECTION("DebugTrace: no memory, stack and storage") {
        DebugTrace debug_trace;
        debug_trace.failed = false;
        debug_trace.gas = 20;
        debug_trace.return_value = "deadbeaf";
        debug_trace.debug_logs.push_back(log);

        debug_trace.debug_config.disableStorage = true;
        debug_trace.debug_config.disableMemory = true;
        debug_trace.debug_config.disableStack = true;

        CHECK(debug_trace == R"({
            "failed": false,
            "gas": 20,
            "returnValue": "deadbeaf",
            "structLogs": [{
                "depth": 1,
                "gas": 3,
                "gasCost": 4,
                "op": "PUSH1",
                "pc": 1
            }]
        })"_json);
    }

    SECTION("DebugTrace: only memory") {
        DebugTrace debug_trace;
        debug_trace.failed = false;
        debug_trace.gas = 20;
        debug_trace.return_value = "deadbeaf";
        debug_trace.debug_logs.push_back(log);

        debug_trace.debug_config.disableStorage = true;
        debug_trace.debug_config.disableMemory = false;
        debug_trace.debug_config.disableStack = true;

        CHECK(debug_trace == R"({
            "failed": false,
            "gas": 20,
            "returnValue": "deadbeaf",
            "structLogs": [{
                "depth": 1,
                "gas": 3,
                "gasCost": 4,
                "op": "PUSH1",
                "pc": 1,
                "memory":["0000000000000000000000000000000000000000000000000000000000000080"]
            }]
        })"_json);
    }

    SECTION("DebugTrace: only stack") {
        DebugTrace debug_trace;
        debug_trace.failed = false;
        debug_trace.gas = 20;
        debug_trace.return_value = "deadbeaf";
        debug_trace.debug_logs.push_back(log);

        debug_trace.debug_config.disableStorage = true;
        debug_trace.debug_config.disableMemory = true;
        debug_trace.debug_config.disableStack = false;

        CHECK(debug_trace == R"({
            "failed": false,
            "gas": 20,
            "returnValue": "deadbeaf",
            "structLogs": [{
                "depth": 1,
                "gas": 3,
                "gasCost": 4,
                "op": "PUSH1",
                "pc": 1,
                "stack":["0x80"]
            }]
        })"_json);
    }

    SECTION("DebugTrace: only storage") {
        DebugTrace debug_trace;
        debug_trace.failed = false;
        debug_trace.gas = 20;
        debug_trace.return_value = "deadbeaf";
        debug_trace.debug_logs.push_back(log);

        debug_trace.debug_config.disableStorage = false;
        debug_trace.debug_config.disableMemory = true;
        debug_trace.debug_config.disableStack = true;

        CHECK(debug_trace == R"({
            "failed": false,
            "gas": 20,
            "returnValue": "deadbeaf",
            "structLogs": [{
                "depth": 1,
                "gas": 3,
                "gasCost": 4,
                "op": "PUSH1",
                "pc": 1,
                "storage": {
                    "804292fe56769f4b9f0e91cf85875f67487cd9e85a084cbba2188be4466c4f23": "0000000000000000000000000000000000000000000000000000000000000008"
                }
            }]
        })"_json);
    }

    SECTION("DebugTrace: full") {
        DebugTrace debug_trace;
        debug_trace.failed = false;
        debug_trace.gas = 20;
        debug_trace.return_value = "deadbeaf";
        debug_trace.debug_logs.push_back(log);

        debug_trace.debug_config.disableStorage = false;
        debug_trace.debug_config.disableMemory = false;
        debug_trace.debug_config.disableStack = false;

        CHECK(debug_trace == R"({
            "failed": false,
            "gas": 20,
            "returnValue": "deadbeaf",
            "structLogs": [{
                "depth": 1,
                "gas": 3,
                "gasCost": 4,
                "op": "PUSH1",
                "pc": 1,
                "stack":["0x80"],
                "memory":["0000000000000000000000000000000000000000000000000000000000000080"],
                "storage": {
                    "804292fe56769f4b9f0e91cf85875f67487cd9e85a084cbba2188be4466c4f23": "0000000000000000000000000000000000000000000000000000000000000008"
                }
            }]
        })"_json);
    }

    SECTION("DebugTrace vector") {
        DebugTrace debug_trace;
        debug_trace.failed = false;
        debug_trace.gas = 20;
        debug_trace.return_value = "deadbeaf";
        debug_trace.debug_logs.push_back(log);

        debug_trace.debug_config.disableStorage = false;
        debug_trace.debug_config.disableMemory = false;
        debug_trace.debug_config.disableStack = false;

        std::vector<DebugTrace> debug_traces;
        debug_traces.push_back(debug_trace);

        CHECK(debug_traces == R"([{
            "failed": false,
            "gas": 20,
            "returnValue": "deadbeaf",
            "structLogs": [{
                "depth": 1,
                "gas": 3,
                "gasCost": 4,
                "op": "PUSH1",
                "pc": 1,
                "stack":["0x80"],
                "memory":["0000000000000000000000000000000000000000000000000000000000000080"],
                "storage": {
                    "804292fe56769f4b9f0e91cf85875f67487cd9e85a084cbba2188be4466c4f23": "0000000000000000000000000000000000000000000000000000000000000008"
                }
            }]
        }])"_json);
    }
}

TEST_CASE("DebugConfig") {
    SILKRPC_LOG_STREAMS(null_stream(), null_stream());
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    SECTION("json deserialization") {
        nlohmann::json json = R"({
            "disableStorage": true,
            "disableMemory": false,
            "disableStack": true
            })"_json;

        DebugConfig config;
        from_json(json, config);

        CHECK(config.disableStorage == true);
        CHECK(config.disableMemory == false);
        CHECK(config.disableStack == true);
    }
    SECTION("dump on stream") {
        DebugConfig config{true, false, true};

        std::ostringstream os;
        os << config;
        CHECK(os.str() == "disableStorage: true disableMemory: false disableStack: true");
    }
}
}  // namespace silkrpc::debug
