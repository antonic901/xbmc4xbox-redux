/*
*      Copyright (C) 2005-2013 Team Kodi
*      http://kodi.tv
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#include <stdlib.h>
#include "InputCodingTableBasePY.h"
#include "utils/CharsetConverter.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"

std::map<std::string, std::wstring> fillCodemap()
{
  std::map<std::string, std::wstring> mapcode;
  mapcode.insert(std::pair<std::string, std::wstring>("a", L"啊阿吖嗄腌锕"));
  mapcode.insert(std::pair<std::string, std::wstring>("ai", L"爱矮挨哎碍癌艾唉哀蔼隘埃皑嗌嫒瑷暧捱砹嗳锿霭"));
  mapcode.insert(std::pair<std::string, std::wstring>("an", L"按安暗岸俺案鞍氨胺庵揞犴铵桉谙鹌埯黯"));
  mapcode.insert(std::pair<std::string, std::wstring>("ang", L"昂肮盎"));
  mapcode.insert(std::pair<std::string, std::wstring>("ao", L"袄凹傲奥熬懊敖翱澳拗媪廒骜嗷坳遨聱螯獒鏊鳌鏖岙"));
  mapcode.insert(std::pair<std::string, std::wstring>("ba", L"把八吧爸拔罢跋巴芭扒坝霸叭靶笆疤耙捌粑茇岜鲅钯魃菝灞"));
  mapcode.insert(std::pair<std::string, std::wstring>("bai", L"百白摆败柏拜佰伯稗捭掰"));
  mapcode.insert(std::pair<std::string, std::wstring>("ban", L"半办班般拌搬版斑板伴扳扮瓣颁绊癍坂钣舨阪瘢"));
  mapcode.insert(std::pair<std::string, std::wstring>("bang", L"帮棒绑磅镑邦榜蚌傍梆膀谤浜蒡"));
  mapcode.insert(std::pair<std::string, std::wstring>("bao", L"包抱报饱保暴薄宝爆剥豹刨雹褒堡苞胞鲍炮龅孢煲褓鸨趵葆勹"));
  mapcode.insert(std::pair<std::string, std::wstring>("bei", L"被北倍杯背悲备碑卑贝辈钡焙狈惫臂褙悖蓓鹎鐾呗邶鞴孛陂碚埤萆"));
  mapcode.insert(std::pair<std::string, std::wstring>("ben", L"本奔苯笨锛贲畚坌"));
  mapcode.insert(std::pair<std::string, std::wstring>("beng", L"蹦绷甭崩迸蚌泵甏嘣堋"));
  mapcode.insert(std::pair<std::string, std::wstring>("bi", L"比笔闭鼻碧必避逼毕臂彼鄙壁蓖币弊辟蔽毙庇敝陛毖痹秘泌秕薜荸芘萆匕裨畀俾嬖狴筚箅篦舭荜襞庳铋跸吡愎贲滗濞璧哔髀弼妣婢埤"));
  mapcode.insert(std::pair<std::string, std::wstring>("bian", L"边变便遍编辩扁贬鞭卞辨辫忭砭匾汴碥蝙褊鳊笾苄窆弁缏煸"));
  mapcode.insert(std::pair<std::string, std::wstring>("biao", L"表标彪膘杓婊飑飙鳔瘭飚镳裱骠镖灬髟"));
  mapcode.insert(std::pair<std::string, std::wstring>("bie", L"别憋鳖瘪蹩"));
  mapcode.insert(std::pair<std::string, std::wstring>("bin", L"宾濒摈彬斌滨豳膑殡缤髌傧槟鬓镔玢"));
  mapcode.insert(std::pair<std::string, std::wstring>("bing", L"并病兵冰丙饼屏秉柄炳摒槟禀邴冫"));
  mapcode.insert(std::pair<std::string, std::wstring>("bo", L"拨波播泊博伯驳玻剥薄勃菠钵搏脖帛柏舶渤铂箔膊卜礴跛檗亳鹁踣啵蕃簸钹饽擘孛百趵"));
  mapcode.insert(std::pair<std::string, std::wstring>("bu", L"不步补布部捕卜簿哺堡埠怖埔瓿逋晡钸钚醭卟"));
  mapcode.insert(std::pair<std::string, std::wstring>("ca", L"擦礤嚓"));
  mapcode.insert(std::pair<std::string, std::wstring>("cai", L"才菜采材财裁猜踩睬蔡彩"));
  mapcode.insert(std::pair<std::string, std::wstring>("can", L"蚕残掺参惨惭餐灿骖璨孱黪粲"));
  mapcode.insert(std::pair<std::string, std::wstring>("cang", L"藏仓沧舱苍伧"));
  mapcode.insert(std::pair<std::string, std::wstring>("cao", L"草操曹槽糙嘈艚螬漕艹"));
  mapcode.insert(std::pair<std::string, std::wstring>("ce", L"册侧策测厕恻"));
  mapcode.insert(std::pair<std::string, std::wstring>("cen", L"参岑涔"));
  mapcode.insert(std::pair<std::string, std::wstring>("ceng", L"曾层蹭噌"));
  mapcode.insert(std::pair<std::string, std::wstring>("cha", L"查插叉茶差岔搽察茬碴刹诧楂槎镲衩汊馇檫姹杈锸嚓猹"));
  mapcode.insert(std::pair<std::string, std::wstring>("chai", L"柴拆差豺钗瘥虿侪龇"));
  mapcode.insert(std::pair<std::string, std::wstring>("chan", L"产缠掺搀阐颤铲谗蝉单馋觇婵蒇谄冁廛孱蟾羼镡忏潺禅躔澶"));
  mapcode.insert(std::pair<std::string, std::wstring>("chang", L"长唱常场厂尝肠畅昌敞倡偿猖鲳氅菖惝嫦徜鬯阊怅伥昶苌娼"));
  mapcode.insert(std::pair<std::string, std::wstring>("chao", L"朝抄超吵潮巢炒嘲剿绰钞怊耖晁"));
  mapcode.insert(std::pair<std::string, std::wstring>("che", L"车撤扯掣彻澈坼砗屮"));
  mapcode.insert(std::pair<std::string, std::wstring>("chen", L"趁称辰臣尘晨沉陈衬忱郴榇抻谌碜谶宸龀嗔琛"));
  mapcode.insert(std::pair<std::string, std::wstring>("cheng", L"成乘盛撑称城程呈诚秤惩逞骋澄橙承塍柽埕铖噌铛酲裎枨蛏丞瞠徵"));
  mapcode.insert(std::pair<std::string, std::wstring>("chi", L"吃尺迟池翅痴赤齿耻持斥侈弛驰炽匙踟坻茌墀饬媸豉褫敕哧瘛蚩啻鸱眵螭篪魑叱彳笞嗤傺"));
  mapcode.insert(std::pair<std::string, std::wstring>("chong", L"冲重虫充宠崇种艟忡舂铳憧茺"));
  mapcode.insert(std::pair<std::string, std::wstring>("chou", L"抽愁臭仇丑稠绸酬筹踌畴瞅惆俦帱瘳雠"));
  mapcode.insert(std::pair<std::string, std::wstring>("chu", L"出处初锄除触橱楚础储畜滁矗搐躇厨雏楮杵刍怵绌亍憷蹰黜蜍樗褚"));
  mapcode.insert(std::pair<std::string, std::wstring>("chuai", L"揣膪嘬搋踹"));
  mapcode.insert(std::pair<std::string, std::wstring>("chuan", L"穿船传串川喘椽氚遄钏舡舛巛"));
  mapcode.insert(std::pair<std::string, std::wstring>("chuang", L"窗床闯创疮幢怆"));
  mapcode.insert(std::pair<std::string, std::wstring>("chui", L"吹垂炊锤捶槌棰陲"));
  mapcode.insert(std::pair<std::string, std::wstring>("chun", L"春唇纯蠢醇淳椿蝽莼鹑"));
  mapcode.insert(std::pair<std::string, std::wstring>("chuo", L"戳绰踔啜龊辍辶"));
  mapcode.insert(std::pair<std::string, std::wstring>("ci", L"次此词瓷慈雌磁辞刺茨伺疵赐差兹呲鹚祠糍粢茈"));
  mapcode.insert(std::pair<std::string, std::wstring>("cong", L"从丛葱匆聪囱琮枞淙璁骢苁"));
  mapcode.insert(std::pair<std::string, std::wstring>("cou", L"凑楱辏腠"));
  mapcode.insert(std::pair<std::string, std::wstring>("cu", L"粗醋簇促徂猝蔟蹙酢殂蹴"));
  mapcode.insert(std::pair<std::string, std::wstring>("cuan", L"窜蹿篡攒汆爨镩撺"));
  mapcode.insert(std::pair<std::string, std::wstring>("cui", L"催脆摧翠崔淬瘁粹璀啐悴萃毳榱隹"));
  mapcode.insert(std::pair<std::string, std::wstring>("cun", L"村寸存忖皴"));
  mapcode.insert(std::pair<std::string, std::wstring>("cuo", L"错撮搓挫措磋嵯厝鹾脞痤蹉瘥锉矬躜"));
  mapcode.insert(std::pair<std::string, std::wstring>("da", L"大答达打搭瘩笪耷哒褡疸怛靼妲沓嗒鞑"));
  mapcode.insert(std::pair<std::string, std::wstring>("dai", L"带代呆戴待袋逮歹贷怠傣大殆呔玳迨岱甙黛骀绐埭"));
  mapcode.insert(std::pair<std::string, std::wstring>("dan", L"但单蛋担弹掸胆淡丹耽旦氮诞郸惮石疸澹瘅萏殚眈聃箪赕儋啖赡"));
  mapcode.insert(std::pair<std::string, std::wstring>("dang", L"当党挡档荡谠铛宕菪凼裆砀"));
  mapcode.insert(std::pair<std::string, std::wstring>("dao", L"到道倒刀岛盗稻捣悼导蹈祷帱纛忉焘氘叨刂"));
  mapcode.insert(std::pair<std::string, std::wstring>("de", L"的地得德锝"));
  mapcode.insert(std::pair<std::string, std::wstring>("dei", L"得"));
  mapcode.insert(std::pair<std::string, std::wstring>("deng", L"等灯邓登澄瞪凳蹬磴镫噔嶝戥簦"));
  mapcode.insert(std::pair<std::string, std::wstring>("di", L"地第底低敌抵滴帝递嫡弟缔堤的涤提笛迪狄翟蒂觌邸谛诋嘀柢骶羝氐棣睇娣荻碲镝坻籴砥"));
  mapcode.insert(std::pair<std::string, std::wstring>("dia", L"嗲"));
  mapcode.insert(std::pair<std::string, std::wstring>("dian", L"点电店殿淀掂颠垫碘惦奠典佃靛滇甸踮钿坫阽癫簟玷巅癜"));
  mapcode.insert(std::pair<std::string, std::wstring>("diao", L"掉钓叼吊雕调刁碉凋铞铫鲷貂"));
  mapcode.insert(std::pair<std::string, std::wstring>("die", L"爹跌叠碟蝶迭谍牒堞瓞揲蹀耋鲽垤喋"));
  mapcode.insert(std::pair<std::string, std::wstring>("ding", L"顶定盯订叮丁钉鼎锭町玎铤腚碇疔仃耵酊啶"));
  mapcode.insert(std::pair<std::string, std::wstring>("diu", L"丢铥"));
  mapcode.insert(std::pair<std::string, std::wstring>("dong", L"动东懂洞冻冬董栋侗恫峒鸫胨胴硐氡岽咚"));
  mapcode.insert(std::pair<std::string, std::wstring>("dou", L"都斗豆逗陡抖痘兜蚪窦篼蔸"));
  mapcode.insert(std::pair<std::string, std::wstring>("du", L"读度毒渡堵独肚镀赌睹杜督都犊妒蠹笃嘟渎椟牍黩髑芏"));
  mapcode.insert(std::pair<std::string, std::wstring>("duan", L"段短断端锻缎椴煅簖"));
  mapcode.insert(std::pair<std::string, std::wstring>("dui", L"对队堆兑碓怼憝"));
  mapcode.insert(std::pair<std::string, std::wstring>("dun", L"吨顿蹲墩敦钝盾囤遁趸沌盹镦礅炖砘"));
  mapcode.insert(std::pair<std::string, std::wstring>("duo", L"多朵夺舵剁垛跺惰堕掇哆驮度躲踱沲咄铎裰哚缍"));
  mapcode.insert(std::pair<std::string, std::wstring>("e", L"饿哦额鹅蛾扼俄讹阿遏峨娥恶厄鄂锇谔垩锷萼苊轭婀莪鳄颚腭愕呃噩鹗屙"));
  mapcode.insert(std::pair<std::string, std::wstring>("ei", L"诶"));
  mapcode.insert(std::pair<std::string, std::wstring>("en", L"恩摁蒽"));
  mapcode.insert(std::pair<std::string, std::wstring>("er", L"而二耳儿饵尔贰洱珥鲕鸸迩铒"));
  mapcode.insert(std::pair<std::string, std::wstring>("fa", L"发法罚伐乏筏阀珐垡砝"));
  mapcode.insert(std::pair<std::string, std::wstring>("fan", L"反饭翻番犯凡帆返泛繁烦贩范樊藩矾钒燔蘩畈蕃蹯梵幡"));
  mapcode.insert(std::pair<std::string, std::wstring>("fang", L"放房防纺芳方访仿坊妨肪钫邡枋舫鲂匚"));
  mapcode.insert(std::pair<std::string, std::wstring>("fei", L"非飞肥费肺废匪吠沸菲诽啡篚蜚腓扉妃斐狒芾悱镄霏翡榧淝鲱绯痱砩"));
  mapcode.insert(std::pair<std::string, std::wstring>("fen", L"分份芬粉坟奋愤纷忿粪酚焚吩氛汾棼瀵鲼玢偾鼢贲"));
  mapcode.insert(std::pair<std::string, std::wstring>("feng", L"风封逢缝蜂丰枫疯冯奉讽凤峰锋烽砜俸酆葑沣唪"));
  mapcode.insert(std::pair<std::string, std::wstring>("fo", L"佛"));
  mapcode.insert(std::pair<std::string, std::wstring>("fou", L"否缶"));
  mapcode.insert(std::pair<std::string, std::wstring>("fu", L"副幅扶浮富福负伏付复服附俯斧赴缚拂夫父符孵敷赋辅府腐腹妇抚覆辐肤氟佛俘傅讣弗涪袱甫釜脯腑阜咐黼砩苻趺跗蚨芾鲋幞茯滏蜉拊菔蝠鳆蝮绂绋赙罘稃匐麸凫桴莩孚馥驸怫祓呋郛芙艴黻哺阝"));
  mapcode.insert(std::pair<std::string, std::wstring>("ga", L"噶夹嘎咖钆伽旮尬尕尜"));
  mapcode.insert(std::pair<std::string, std::wstring>("gai", L"该改盖概钙芥溉戤垓丐陔赅胲"));
  mapcode.insert(std::pair<std::string, std::wstring>("gan", L"赶干感敢竿甘肝柑杆赣秆旰酐矸疳泔苷擀绀橄澉淦尴坩"));
  mapcode.insert(std::pair<std::string, std::wstring>("gang", L"刚钢纲港缸岗杠冈肛扛筻罡戆"));
  mapcode.insert(std::pair<std::string, std::wstring>("gao", L"高搞告稿膏篙羔糕镐皋郜诰杲缟睾槔锆槁藁"));
  mapcode.insert(std::pair<std::string, std::wstring>("ge", L"个各歌割哥搁格阁隔革咯胳葛蛤戈鸽疙盖合铬骼袼塥虼圪镉仡舸鬲嗝膈搿纥哿铪"));
  mapcode.insert(std::pair<std::string, std::wstring>("gei", L"给"));
  mapcode.insert(std::pair<std::string, std::wstring>("gen", L"跟根哏茛亘艮"));
  mapcode.insert(std::pair<std::string, std::wstring>("geng", L"更耕颈梗耿庚羹埂赓鲠哽绠"));
  mapcode.insert(std::pair<std::string, std::wstring>("gong", L"工公功共弓攻宫供恭拱贡躬巩汞龚肱觥珙蚣廾"));
  mapcode.insert(std::pair<std::string, std::wstring>("gou", L"够沟狗钩勾购构苟垢岣彀枸鞲觏缑笱诟遘媾篝佝"));
  mapcode.insert(std::pair<std::string, std::wstring>("gu", L"古股鼓谷故孤箍姑顾固雇估咕骨辜沽蛊贾菇梏鸪汩轱崮菰鹄鹘钴臌酤鲴诂牯瞽毂锢牿痼觚蛄罟嘏"));
  mapcode.insert(std::pair<std::string, std::wstring>("gua", L"挂刮瓜寡剐褂卦呱胍鸹栝诖"));
  mapcode.insert(std::pair<std::string, std::wstring>("guai", L"怪拐乖掴"));
  mapcode.insert(std::pair<std::string, std::wstring>("guan", L"关管官观馆惯罐灌冠贯棺纶盥莞掼涫鳏鹳倌"));
  mapcode.insert(std::pair<std::string, std::wstring>("guang", L"光广逛桄犷咣胱"));
  mapcode.insert(std::pair<std::string, std::wstring>("gui", L"归贵鬼跪轨规硅桂柜龟诡闺瑰圭刽癸炔庋宄桧刿鳜鲑皈匦妫晷簋炅"));
  mapcode.insert(std::pair<std::string, std::wstring>("gun", L"滚棍辊鲧衮磙绲"));
  mapcode.insert(std::pair<std::string, std::wstring>("guo", L"过国果裹锅郭涡埚椁聒馘猓崞掴帼呙虢蜾蝈锞"));
  mapcode.insert(std::pair<std::string, std::wstring>("ha", L"哈蛤铪"));
  mapcode.insert(std::pair<std::string, std::wstring>("hai", L"还海害咳氦孩骇咳骸亥嗨醢胲"));
  mapcode.insert(std::pair<std::string, std::wstring>("han", L"喊含汗寒汉旱酣韩焊涵函憨翰罕撼捍憾悍邯邗菡撖瀚顸蚶焓颔晗鼾"));
  mapcode.insert(std::pair<std::string, std::wstring>("hang", L"行巷航夯杭吭颃沆绗"));
  mapcode.insert(std::pair<std::string, std::wstring>("hao", L"好号浩嚎壕郝毫豪耗貉镐昊颢灏嚆蚝嗥皓蒿濠薅"));
  mapcode.insert(std::pair<std::string, std::wstring>("he", L"和喝合河禾核何呵荷贺赫褐盒鹤菏貉阂涸吓嗬劾盍翮阖颌壑诃纥曷蚵"));
  mapcode.insert(std::pair<std::string, std::wstring>("hei", L"黑嘿"));
  mapcode.insert(std::pair<std::string, std::wstring>("hen", L"很狠恨痕"));
  mapcode.insert(std::pair<std::string, std::wstring>("heng", L"横恒哼衡亨桁珩蘅"));
  mapcode.insert(std::pair<std::string, std::wstring>("hong", L"红轰哄虹洪宏烘鸿弘讧訇蕻闳薨黉荭泓"));
  mapcode.insert(std::pair<std::string, std::wstring>("hou", L"后厚吼喉侯候猴鲎篌堠後逅糇骺瘊"));
  mapcode.insert(std::pair<std::string, std::wstring>("hu", L"湖户呼虎壶互胡护糊弧忽狐蝴葫沪乎核瑚唬鹕冱怙鹱笏戽扈鹘浒祜醐琥囫烀轷瓠煳斛鹄猢惚岵滹觳唿槲虍"));
  mapcode.insert(std::pair<std::string, std::wstring>("hua", L"话花化画华划滑哗猾铧桦骅砉"));
  mapcode.insert(std::pair<std::string, std::wstring>("huai", L"坏怀淮槐徊踝"));
  mapcode.insert(std::pair<std::string, std::wstring>("huan", L"换还唤环患缓欢幻宦涣焕豢桓痪漶獾擐逭鲩郇鬟寰奂锾圜洹萑缳浣垸"));
  mapcode.insert(std::pair<std::string, std::wstring>("huang", L"黄慌晃荒簧凰皇谎惶蝗磺恍煌幌隍肓潢篁徨鳇遑癀湟蟥璜"));
  mapcode.insert(std::pair<std::string, std::wstring>("hui", L"回会灰绘挥汇辉毁悔惠晦徽恢秽慧贿蛔讳卉烩诲彗浍蕙喙恚哕晖隳麾诙蟪茴洄咴虺荟缋桧"));
  mapcode.insert(std::pair<std::string, std::wstring>("hun", L"混昏荤浑婚魂阍珲馄溷诨"));
  mapcode.insert(std::pair<std::string, std::wstring>("huo", L"或活火伙货和获祸豁霍惑嚯镬耠劐藿攉锪蠖钬夥"));
  mapcode.insert(std::pair<std::string, std::wstring>("ji", L"几及急既即机鸡积记级极计挤己季寄纪系基激吉脊际汲肌嫉姬绩缉饥迹棘蓟技冀辑伎祭剂悸济籍寂奇忌妓继集给击圾箕讥畸稽疾墼洎鲚屐齑戟鲫嵇矶稷戢虮笈暨笄剞叽蒺跻嵴掎跽霁唧畿荠瘠玑羁丌偈芨佶赍楫髻咭蕺觊麂骥殛岌亟犄乩芰哜彐萁藉"));
  mapcode.insert(std::pair<std::string, std::wstring>("jia", L"家加假价架甲佳夹嘉驾嫁枷荚颊钾稼茄贾铗葭迦戛浃镓痂恝岬跏嘏伽胛笳珈瘕郏袈蛱袷铪"));
  mapcode.insert(std::pair<std::string, std::wstring>("jian", L"见件减尖间键贱肩兼建检箭煎简剪歼监坚奸健艰荐剑渐溅涧鉴践捡柬笺俭碱硷拣舰槛缄茧饯翦鞯戋谏牮枧腱趼缣搛戬毽菅鲣笕谫楗囝蹇裥踺睑謇鹣蒹僭锏湔犍谮"));
  mapcode.insert(std::pair<std::string, std::wstring>("jiang", L"将讲江奖降浆僵姜酱蒋疆匠强桨虹豇礓缰犟耩绛茳糨洚"));
  mapcode.insert(std::pair<std::string, std::wstring>("jiao", L"叫脚交角教较缴觉焦胶娇绞校搅骄狡浇矫郊嚼蕉轿窖椒礁饺铰酵侥剿徼艽僬蛟敫峤跤姣皎茭鹪噍醮佼鲛挢"));
  mapcode.insert(std::pair<std::string, std::wstring>("jie", L"接节街借皆截解界结届姐揭戒介阶劫芥竭洁疥藉秸桔杰捷诫睫偈桀喈拮骱羯蚧嗟颉鲒婕碣讦孑疖诘卩锴"));
  mapcode.insert(std::pair<std::string, std::wstring>("jin", L"进近今仅紧金斤尽劲禁浸锦晋筋津谨巾襟烬靳廑瑾馑槿衿堇荩矜噤缙卺妗赆觐钅"));
  mapcode.insert(std::pair<std::string, std::wstring>("jing", L"竟静井惊经镜京净敬精景警竞境径荆晶鲸粳颈兢茎睛劲痉靖肼獍阱腈弪刭憬婧胫菁儆旌迳靓泾陉"));
  mapcode.insert(std::pair<std::string, std::wstring>("jiong", L"窘炯扃迥冂"));
  mapcode.insert(std::pair<std::string, std::wstring>("jiu", L"就九酒旧久揪救纠舅究韭厩臼玖灸咎疚赳鹫僦柩桕鬏鸠阄啾"));
  mapcode.insert(std::pair<std::string, std::wstring>("ju", L"句举巨局具距锯剧居聚拘菊矩沮拒惧鞠狙驹据柜俱车咀疽踞炬倨醵裾屦犋苴窭飓锔椐苣琚掬榘龃趄莒雎遽橘踽榉鞫钜讵枸瞿蘧"));
  mapcode.insert(std::pair<std::string, std::wstring>("juan", L"卷圈倦鹃捐娟眷绢鄄锩蠲镌狷桊涓隽"));
  mapcode.insert(std::pair<std::string, std::wstring>("jue", L"决绝觉角爵掘诀撅倔抉攫嚼脚桷橛觖劂爝矍镢獗珏崛蕨噘谲蹶孓厥阙"));
  mapcode.insert(std::pair<std::string, std::wstring>("jun", L"军君均菌俊峻龟竣骏钧浚郡筠麇皲捃"));
  mapcode.insert(std::pair<std::string, std::wstring>("ka", L"卡喀咯咖胩咔佧"));
  mapcode.insert(std::pair<std::string, std::wstring>("kai", L"开揩凯慨楷垲剀锎铠锴忾恺蒈"));
  mapcode.insert(std::pair<std::string, std::wstring>("kan", L"看砍堪刊坎槛勘龛戡侃瞰莰阚凵"));
  mapcode.insert(std::pair<std::string, std::wstring>("kang", L"抗炕扛糠康慷亢钪闶伉"));
  mapcode.insert(std::pair<std::string, std::wstring>("kao", L"靠考烤拷栲犒尻铐"));
  mapcode.insert(std::pair<std::string, std::wstring>("ke", L"可克棵科颗刻课客壳渴苛柯磕咳坷呵恪岢蝌缂蚵轲窠钶氪颏瞌锞稞珂髁疴嗑溘骒铪"));
  mapcode.insert(std::pair<std::string, std::wstring>("ken", L"肯啃恳垦裉"));
  mapcode.insert(std::pair<std::string, std::wstring>("keng", L"坑吭铿胫铒"));
  mapcode.insert(std::pair<std::string, std::wstring>("kong", L"空孔控恐倥崆箜"));
  mapcode.insert(std::pair<std::string, std::wstring>("kou", L"口扣抠寇蔻芤眍筘叩"));
  mapcode.insert(std::pair<std::string, std::wstring>("ku", L"哭库苦枯裤窟酷刳骷喾堀绔"));
  mapcode.insert(std::pair<std::string, std::wstring>("kua", L"跨垮挎夸胯侉锞"));
  mapcode.insert(std::pair<std::string, std::wstring>("kuai", L"快块筷会侩哙蒯郐狯脍"));
  mapcode.insert(std::pair<std::string, std::wstring>("kuan", L"宽款髋"));
  mapcode.insert(std::pair<std::string, std::wstring>("kuang", L"矿筐狂框况旷匡眶诳邝纩夼诓圹贶哐"));
  mapcode.insert(std::pair<std::string, std::wstring>("kui", L"亏愧奎窥溃葵魁馈盔傀岿匮愦揆睽跬聩篑喹逵暌蒉悝喟馗蝰隗夔"));
  mapcode.insert(std::pair<std::string, std::wstring>("kun", L"捆困昆坤鲲锟髡琨醌阃悃顽"));
  mapcode.insert(std::pair<std::string, std::wstring>("kuo", L"阔扩括廓蛞"));
  mapcode.insert(std::pair<std::string, std::wstring>("la", L"拉啦辣蜡腊喇垃落瘌邋砬剌旯"));
  mapcode.insert(std::pair<std::string, std::wstring>("lai", L"来赖莱濑赉崃涞铼籁徕癞睐"));
  mapcode.insert(std::pair<std::string, std::wstring>("lan", L"蓝兰烂拦篮懒栏揽缆滥阑谰婪澜览榄岚褴镧斓罱漤"));
  mapcode.insert(std::pair<std::string, std::wstring>("lang", L"浪狼廊郎朗榔琅稂螂莨啷锒阆蒗"));
  mapcode.insert(std::pair<std::string, std::wstring>("lao", L"老捞牢劳烙涝落姥酪络佬耢铹醪铑唠栳崂痨"));
  mapcode.insert(std::pair<std::string, std::wstring>("le", L"了乐勒鳓仂叻泐"));
  mapcode.insert(std::pair<std::string, std::wstring>("lei", L"类累泪雷垒勒擂蕾肋镭儡磊缧诔耒酹羸嫘檑嘞"));
  mapcode.insert(std::pair<std::string, std::wstring>("leng", L"冷棱楞愣塄"));
  mapcode.insert(std::pair<std::string, std::wstring>("li", L"里离力立李例哩理利梨厘礼历丽吏砾漓莉傈荔俐痢狸粒沥隶栗璃鲤厉励犁黎篱郦鹂笠坜苈鳢缡跞蜊锂澧粝蓠枥蠡鬲呖砺嫠篥疠疬猁藜溧鲡戾栎唳醴轹詈骊罹逦俪喱雳黧莅俚蛎娌砬"));
  mapcode.insert(std::pair<std::string, std::wstring>("lia", L"俩"));
  mapcode.insert(std::pair<std::string, std::wstring>("lian", L"连联练莲恋脸炼链敛怜廉帘镰涟蠊琏殓蔹鲢奁潋臁裢濂裣楝"));
  mapcode.insert(std::pair<std::string, std::wstring>("liang", L"两亮辆凉粮梁量良晾谅俩粱墚踉椋魉莨"));
  mapcode.insert(std::pair<std::string, std::wstring>("liao", L"了料撩聊撂疗廖燎辽僚寥镣潦钌蓼尥寮缭獠鹩嘹"));
  mapcode.insert(std::pair<std::string, std::wstring>("lie", L"列裂猎劣烈咧埒捩鬣趔躐冽洌"));
  mapcode.insert(std::pair<std::string, std::wstring>("lin", L"林临淋邻磷鳞赁吝拎琳霖凛遴嶙蔺粼麟躏辚廪懔瞵檩膦啉"));
  mapcode.insert(std::pair<std::string, std::wstring>("ling", L"另令领零铃玲灵岭龄凌陵菱伶羚棱翎蛉苓绫瓴酃呤泠棂柃鲮聆囹"));
  mapcode.insert(std::pair<std::string, std::wstring>("liu", L"六流留刘柳溜硫瘤榴琉馏碌陆绺锍鎏镏浏骝旒鹨熘遛"));
  mapcode.insert(std::pair<std::string, std::wstring>("lo", L"咯"));
  mapcode.insert(std::pair<std::string, std::wstring>("long", L"龙拢笼聋隆垄弄咙窿陇垅胧珑茏泷栊癃砻"));
  mapcode.insert(std::pair<std::string, std::wstring>("lou", L"楼搂漏陋露娄篓偻蝼镂蒌耧髅喽瘘嵝"));
  mapcode.insert(std::pair<std::string, std::wstring>("lu", L"路露录鹿陆炉卢鲁卤芦颅庐碌掳绿虏赂戮潞禄麓六鲈栌渌逯泸轳氇簏橹辂垆胪噜镥辘漉撸璐鸬鹭舻"));
  mapcode.insert(std::pair<std::string, std::wstring>("luan", L"乱卵滦峦孪挛栾銮脔娈鸾"));
  mapcode.insert(std::pair<std::string, std::wstring>("lue", L"略掠锊"));
  mapcode.insert(std::pair<std::string, std::wstring>("lun", L"论轮抡伦沦仑纶囵"));
  mapcode.insert(std::pair<std::string, std::wstring>("luo", L"落罗锣裸骡烙箩螺萝洛骆逻络咯荦漯蠃雒倮硌椤捋脶瘰摞泺珞镙猡铬"));
  mapcode.insert(std::pair<std::string, std::wstring>("lv", L"绿率铝驴旅屡滤吕律氯缕侣虑履偻膂榈闾捋褛稆"));
  mapcode.insert(std::pair<std::string, std::wstring>("lve", L"略掠锊"));
  mapcode.insert(std::pair<std::string, std::wstring>("m", L"呒"));
  mapcode.insert(std::pair<std::string, std::wstring>("ma", L"吗妈马嘛麻骂抹码玛蚂摩唛蟆犸嬷杩"));
  mapcode.insert(std::pair<std::string, std::wstring>("mai", L"买卖迈埋麦脉劢霾荬"));
  mapcode.insert(std::pair<std::string, std::wstring>("man", L"满慢瞒漫蛮蔓曼馒埋谩幔鳗墁螨镘颟鞔缦熳"));
  mapcode.insert(std::pair<std::string, std::wstring>("mang", L"忙芒盲莽茫氓硭邙蟒漭"));
  mapcode.insert(std::pair<std::string, std::wstring>("mao", L"毛冒帽猫矛卯貌茂贸铆锚茅耄茆瑁蝥髦懋昴牦瞀峁袤蟊旄泖"));
  mapcode.insert(std::pair<std::string, std::wstring>("me", L"么"));
  mapcode.insert(std::pair<std::string, std::wstring>("mei", L"没每煤镁美酶妹枚霉玫眉梅寐昧媒媚嵋猸袂湄浼鹛莓魅镅楣"));
  mapcode.insert(std::pair<std::string, std::wstring>("men", L"门们闷懑扪钔焖"));
  mapcode.insert(std::pair<std::string, std::wstring>("meng", L"猛梦蒙锰孟盟檬萌礞蜢勐懵甍蠓虻朦艋艨瞢"));
  mapcode.insert(std::pair<std::string, std::wstring>("mi", L"米密迷眯蜜谜觅秘弥幂靡糜泌醚蘼縻咪汨麋祢猕弭谧芈脒宓敉嘧糸冖"));
  mapcode.insert(std::pair<std::string, std::wstring>("mian", L"面棉免绵眠缅勉冕娩腼湎眄沔渑宀"));
  mapcode.insert(std::pair<std::string, std::wstring>("miao", L"秒苗庙妙描瞄藐渺眇缪缈淼喵杪鹋邈"));
  mapcode.insert(std::pair<std::string, std::wstring>("mie", L"灭蔑咩篾蠛乜"));
  mapcode.insert(std::pair<std::string, std::wstring>("min", L"民抿敏闽皿悯珉愍缗闵玟苠泯黾鳘岷"));
  mapcode.insert(std::pair<std::string, std::wstring>("ming", L"名明命鸣铭螟冥瞑暝茗溟酩"));
  mapcode.insert(std::pair<std::string, std::wstring>("miu", L"谬缪"));
  mapcode.insert(std::pair<std::string, std::wstring>("mo", L"摸磨抹末膜墨没莫默魔模摩摹漠陌蘑脉沫万寞秣瘼殁镆嫫谟蓦貊貘麽茉馍耱"));
  mapcode.insert(std::pair<std::string, std::wstring>("mou", L"某谋牟眸蛑鍪侔缪哞"));
  mapcode.insert(std::pair<std::string, std::wstring>("mu", L"木母亩幕目墓牧牟模穆暮牡拇募慕睦姆钼毪坶沐仫苜"));
  mapcode.insert(std::pair<std::string, std::wstring>("na", L"那拿哪纳钠娜呐衲捺镎肭"));
  mapcode.insert(std::pair<std::string, std::wstring>("nai", L"乃耐奶奈氖萘艿柰鼐佴"));
  mapcode.insert(std::pair<std::string, std::wstring>("nan", L"难南男赧囡蝻楠喃腩"));
  mapcode.insert(std::pair<std::string, std::wstring>("nang", L"囊馕曩囔攮"));
  mapcode.insert(std::pair<std::string, std::wstring>("nao", L"闹脑恼挠淖孬铙瑙垴呶蛲猱硇"));
  mapcode.insert(std::pair<std::string, std::wstring>("ne", L"呢哪讷"));
  mapcode.insert(std::pair<std::string, std::wstring>("nei", L"内馁"));
  mapcode.insert(std::pair<std::string, std::wstring>("nen", L"嫩恁"));
  mapcode.insert(std::pair<std::string, std::wstring>("neng", L"能"));
  mapcode.insert(std::pair<std::string, std::wstring>("ng", L"嗯"));
  mapcode.insert(std::pair<std::string, std::wstring>("ni", L"你泥拟腻逆呢溺倪尼匿妮霓铌昵坭祢猊伲怩鲵睨旎慝"));
  mapcode.insert(std::pair<std::string, std::wstring>("nian", L"年念捻撵拈碾蔫廿黏辇鲇鲶埝"));
  mapcode.insert(std::pair<std::string, std::wstring>("niang", L"娘酿"));
  mapcode.insert(std::pair<std::string, std::wstring>("niao", L"鸟尿袅茑脲嬲"));
  mapcode.insert(std::pair<std::string, std::wstring>("nie", L"捏镍聂孽涅镊啮陧蘖嗫臬蹑颞乜"));
  mapcode.insert(std::pair<std::string, std::wstring>("nin", L"您"));
  mapcode.insert(std::pair<std::string, std::wstring>("ning", L"拧凝宁柠狞泞佞甯咛聍"));
  mapcode.insert(std::pair<std::string, std::wstring>("niu", L"牛扭纽钮拗妞狃忸"));
  mapcode.insert(std::pair<std::string, std::wstring>("nong", L"弄浓农脓哝侬"));
  mapcode.insert(std::pair<std::string, std::wstring>("nou", L"耨"));
  mapcode.insert(std::pair<std::string, std::wstring>("nu", L"怒努奴孥胬驽弩"));
  mapcode.insert(std::pair<std::string, std::wstring>("nuan", L"暖"));
  mapcode.insert(std::pair<std::string, std::wstring>("nue", L"虐疟"));
  mapcode.insert(std::pair<std::string, std::wstring>("nuo", L"挪诺懦糯娜喏傩锘搦"));
  mapcode.insert(std::pair<std::string, std::wstring>("nv", L"女衄钕恧"));
  mapcode.insert(std::pair<std::string, std::wstring>("nve", L"虐疟"));
  mapcode.insert(std::pair<std::string, std::wstring>("o", L"哦喔噢"));
  mapcode.insert(std::pair<std::string, std::wstring>("ou", L"偶呕欧藕鸥区沤殴怄瓯讴耦"));
  mapcode.insert(std::pair<std::string, std::wstring>("pa", L"怕爬趴啪耙扒帕琶筢杷葩"));
  mapcode.insert(std::pair<std::string, std::wstring>("pai", L"派排拍牌迫徘湃哌俳蒎"));
  mapcode.insert(std::pair<std::string, std::wstring>("pan", L"盘盼判攀畔潘叛磐番胖襻蟠袢泮拚爿蹒"));
  mapcode.insert(std::pair<std::string, std::wstring>("pang", L"旁胖耪庞乓膀磅滂彷逄螃镑"));
  mapcode.insert(std::pair<std::string, std::wstring>("pao", L"跑抛炮泡刨袍咆狍匏庖疱脬"));
  mapcode.insert(std::pair<std::string, std::wstring>("pei", L"陪配赔呸胚佩培沛裴旆锫帔醅霈辔"));
  mapcode.insert(std::pair<std::string, std::wstring>("pen", L"喷盆湓"));
  mapcode.insert(std::pair<std::string, std::wstring>("peng", L"碰捧棚砰蓬朋彭鹏烹硼膨抨澎篷怦堋蟛嘭"));
  mapcode.insert(std::pair<std::string, std::wstring>("pi", L"批皮披匹劈辟坯屁脾僻疲痞霹琵毗啤譬砒否貔丕圮媲癖仳擗郫甓枇睥蜱鼙邳陂铍庀罴埤纰陴淠噼蚍裨疋芘"));
  mapcode.insert(std::pair<std::string, std::wstring>("pian", L"片篇骗偏便扁翩缏犏骈胼蹁谝"));
  mapcode.insert(std::pair<std::string, std::wstring>("piao", L"忄票飘漂瓢朴螵嫖瞟殍缥嘌骠剽"));
  mapcode.insert(std::pair<std::string, std::wstring>("pie", L"瞥撇氕苤丿"));
  mapcode.insert(std::pair<std::string, std::wstring>("pin", L"品贫聘拼频嫔榀姘牝颦"));
  mapcode.insert(std::pair<std::string, std::wstring>("ping", L"平凭瓶评屏乒萍苹坪冯娉鲆枰俜"));
  mapcode.insert(std::pair<std::string, std::wstring>("po", L"破坡颇婆泼迫泊魄朴繁粕笸皤钋陂鄱攴叵珀钷"));
  mapcode.insert(std::pair<std::string, std::wstring>("pou", L"剖掊裒"));
  mapcode.insert(std::pair<std::string, std::wstring>("pu", L"扑铺谱脯仆蒲葡朴菩曝莆瀑埔圃浦堡普暴镨噗匍溥濮氆蹼璞镤"));
  mapcode.insert(std::pair<std::string, std::wstring>("qi", L"起其七气期齐器妻骑汽棋奇欺漆启戚柒岂砌弃泣祁凄企乞契歧祈栖畦脐崎稽迄缉沏讫旗祺颀骐屺岐蹊蕲桤憩芪荠萋芑汔亟鳍俟槭嘁蛴綦亓欹琪麒琦蜞圻杞葺碛淇耆绮綮"));
  mapcode.insert(std::pair<std::string, std::wstring>("qia", L"恰卡掐洽髂袷葜"));
  mapcode.insert(std::pair<std::string, std::wstring>("qian", L"前钱千牵浅签欠铅嵌钎迁钳乾谴谦潜歉纤扦遣黔堑仟岍钤褰箝掮搴倩慊悭愆虔芡荨缱佥芊阡肷茜椠犍骞羟赶"));
  mapcode.insert(std::pair<std::string, std::wstring>("qiang", L"强枪墙抢腔呛羌蔷蜣跄戗襁戕炝镪锵羟樯嫱"));
  mapcode.insert(std::pair<std::string, std::wstring>("qiao", L"桥瞧敲巧翘锹壳鞘撬悄俏窍雀乔侨峭橇樵荞跷硗憔谯鞒愀缲诮劁峤搞铫"));
  mapcode.insert(std::pair<std::string, std::wstring>("qie", L"切且怯窃茄郄趄惬锲妾箧慊伽挈"));
  mapcode.insert(std::pair<std::string, std::wstring>("qin", L"亲琴侵勤擒寝秦芹沁禽钦吣覃矜衾芩廑嗪螓噙揿檎锓"));
  mapcode.insert(std::pair<std::string, std::wstring>("qing", L"请轻清青情晴氢倾庆擎顷亲卿氰圊謦檠箐苘蜻黥罄鲭磬綮"));
  mapcode.insert(std::pair<std::string, std::wstring>("qiong", L"穷琼跫穹邛蛩茕銎筇"));
  mapcode.insert(std::pair<std::string, std::wstring>("qiu", L"求球秋丘泅仇邱囚酋龟楸蚯裘糗蝤巯逑俅虬赇鳅犰湫遒"));
  mapcode.insert(std::pair<std::string, std::wstring>("qu", L"去取区娶渠曲趋趣屈驱蛆躯龋戌蠼蘧祛蕖磲劬诎鸲阒麴癯衢黢璩氍觑蛐朐瞿岖苣"));
  mapcode.insert(std::pair<std::string, std::wstring>("quan", L"全权劝圈拳犬泉券颧痊醛铨筌绻诠辁畎鬈悛蜷荃犭"));
  mapcode.insert(std::pair<std::string, std::wstring>("que", L"却缺确雀瘸鹊炔榷阙阕悫"));
  mapcode.insert(std::pair<std::string, std::wstring>("qun", L"群裙麇逡"));
  mapcode.insert(std::pair<std::string, std::wstring>("ran", L"染燃然冉髯苒蚺"));
  mapcode.insert(std::pair<std::string, std::wstring>("rang", L"让嚷瓤攘壤穰禳"));
  mapcode.insert(std::pair<std::string, std::wstring>("rao", L"饶绕扰荛桡娆"));
  mapcode.insert(std::pair<std::string, std::wstring>("re", L"热惹喏"));
  mapcode.insert(std::pair<std::string, std::wstring>("ren", L"人任忍认刃仁韧妊纫壬饪轫仞荏葚衽稔亻"));
  mapcode.insert(std::pair<std::string, std::wstring>("reng", L"仍扔"));
  mapcode.insert(std::pair<std::string, std::wstring>("ri", L"日"));
  mapcode.insert(std::pair<std::string, std::wstring>("rong", L"容绒融溶熔荣戎蓉冗茸榕狨嵘肜蝾"));
  mapcode.insert(std::pair<std::string, std::wstring>("rou", L"肉揉柔糅蹂鞣"));
  mapcode.insert(std::pair<std::string, std::wstring>("ru", L"如入汝儒茹乳褥辱蠕孺蓐襦铷嚅缛濡薷颥溽洳"));
  mapcode.insert(std::pair<std::string, std::wstring>("ruan", L"软阮朊"));
  mapcode.insert(std::pair<std::string, std::wstring>("rui", L"瑞蕊锐睿芮蚋枘蕤"));
  mapcode.insert(std::pair<std::string, std::wstring>("run", L"润闰"));
  mapcode.insert(std::pair<std::string, std::wstring>("ruo", L"若弱箬偌"));
  mapcode.insert(std::pair<std::string, std::wstring>("sa", L"撒洒萨仨卅飒脎"));
  mapcode.insert(std::pair<std::string, std::wstring>("sai", L"塞腮鳃赛噻"));
  mapcode.insert(std::pair<std::string, std::wstring>("san", L"三散伞叁馓糁毵"));
  mapcode.insert(std::pair<std::string, std::wstring>("sang", L"桑丧嗓颡磉搡"));
  mapcode.insert(std::pair<std::string, std::wstring>("sao", L"扫嫂搔骚埽鳋臊缫瘙"));
  mapcode.insert(std::pair<std::string, std::wstring>("se", L"色涩瑟塞啬铯穑"));
  mapcode.insert(std::pair<std::string, std::wstring>("sen", L"森"));
  mapcode.insert(std::pair<std::string, std::wstring>("seng", L"僧"));
  mapcode.insert(std::pair<std::string, std::wstring>("sha", L"杀沙啥纱傻砂刹莎厦煞杉唼鲨霎铩痧裟歃"));
  mapcode.insert(std::pair<std::string, std::wstring>("shai", L"晒筛色"));
  mapcode.insert(std::pair<std::string, std::wstring>("shan", L"山闪衫善扇杉删煽单珊掺赡栅苫膳陕汕擅缮嬗蟮芟禅跚鄯潸鳝姗剡骟疝膻讪钐舢埏彡髟"));
  mapcode.insert(std::pair<std::string, std::wstring>("shang", L"上伤尚商赏晌墒裳熵觞绱殇垧"));
  mapcode.insert(std::pair<std::string, std::wstring>("shao", L"少烧捎哨勺梢稍邵韶绍芍鞘苕劭潲艄蛸筲"));
  mapcode.insert(std::pair<std::string, std::wstring>("she", L"社射蛇设舌摄舍折涉赊赦慑奢歙厍畲猞麝滠佘揲"));
  mapcode.insert(std::pair<std::string, std::wstring>("shei", L"谁"));
  mapcode.insert(std::pair<std::string, std::wstring>("shen", L"身伸深婶神甚渗肾审申沈绅呻参砷什娠慎葚莘诜谂矧椹渖蜃哂胂"));
  mapcode.insert(std::pair<std::string, std::wstring>("sheng", L"声省剩生升绳胜盛圣甥牲乘晟渑眚笙嵊"));
  mapcode.insert(std::pair<std::string, std::wstring>("shi", L"是使十时事室市石师试史式识虱矢拾屎驶始似示士世柿匙拭誓逝势什殖峙嗜噬失适仕侍释饰氏狮食恃蚀视实施湿诗尸豕莳埘铈舐鲥鲺贳轼蓍筮炻谥弑酾螫礻铊饣"));
  mapcode.insert(std::pair<std::string, std::wstring>("shou", L"手受收首守瘦授兽售寿艏狩绶扌"));
  mapcode.insert(std::pair<std::string, std::wstring>("shu", L"书树数熟输梳叔属束术述蜀黍鼠淑赎孰蔬疏戍竖墅庶薯漱恕枢暑殊抒曙署舒姝摅秫纾沭毹腧塾菽殳澍倏疋镯"));
  mapcode.insert(std::pair<std::string, std::wstring>("shua", L"刷耍唰"));
  mapcode.insert(std::pair<std::string, std::wstring>("shuai", L"摔甩率帅衰蟀"));
  mapcode.insert(std::pair<std::string, std::wstring>("shuan", L"栓拴闩涮"));
  mapcode.insert(std::pair<std::string, std::wstring>("shuang", L"双霜爽泷孀"));
  mapcode.insert(std::pair<std::string, std::wstring>("shui", L"水睡税说氵"));
  mapcode.insert(std::pair<std::string, std::wstring>("shun", L"顺吮瞬舜"));
  mapcode.insert(std::pair<std::string, std::wstring>("shuo", L"说数硕烁朔搠妁槊蒴铄"));
  mapcode.insert(std::pair<std::string, std::wstring>("si", L"四死丝撕似私嘶思寺司斯伺肆饲嗣巳耜驷兕蛳厮汜锶泗笥咝鸶姒厶缌祀澌俟徙"));
  mapcode.insert(std::pair<std::string, std::wstring>("song", L"送松耸宋颂诵怂讼竦菘淞悚嵩凇崧忪"));
  mapcode.insert(std::pair<std::string, std::wstring>("sou", L"艘搜擞嗽嗾嗖飕叟薮锼馊瞍溲螋"));
  mapcode.insert(std::pair<std::string, std::wstring>("su", L"素速诉塑宿俗苏肃粟酥缩溯僳愫簌觫稣夙嗉谡蔌涑"));
  mapcode.insert(std::pair<std::string, std::wstring>("suan", L"酸算蒜狻"));
  mapcode.insert(std::pair<std::string, std::wstring>("sui", L"岁随碎虽穗遂尿隋髓绥隧祟眭谇濉邃燧荽睢"));
  mapcode.insert(std::pair<std::string, std::wstring>("sun", L"孙损笋榫荪飧狲隼"));
  mapcode.insert(std::pair<std::string, std::wstring>("suo", L"所缩锁琐索梭蓑莎唆挲睃嗍唢桫嗦娑羧"));
  mapcode.insert(std::pair<std::string, std::wstring>("ta", L"他她它踏塔塌拓獭挞蹋溻趿鳎沓榻漯遢铊闼"));
  mapcode.insert(std::pair<std::string, std::wstring>("tai", L"太抬台态胎苔泰酞汰炱肽跆鲐钛薹邰骀"));
  mapcode.insert(std::pair<std::string, std::wstring>("tan", L"谈叹探滩弹碳摊潭贪坛痰毯坦炭瘫谭坍檀袒钽郯镡锬覃澹昙忐赕"));
  mapcode.insert(std::pair<std::string, std::wstring>("tang", L"躺趟堂糖汤塘烫倘淌唐搪棠膛螳樘羰醣瑭镗傥饧溏耥帑铴螗铛"));
  mapcode.insert(std::pair<std::string, std::wstring>("tao", L"套掏逃桃讨淘涛滔陶绦萄鼗洮焘啕饕韬叨"));
  mapcode.insert(std::pair<std::string, std::wstring>("te", L"特铽忑忒"));
  mapcode.insert(std::pair<std::string, std::wstring>("teng", L"疼腾藤誊滕"));
  mapcode.insert(std::pair<std::string, std::wstring>("ti", L"提替体题踢蹄剃剔梯锑啼涕嚏惕屉醍鹈绨缇倜裼逖荑悌"));
  mapcode.insert(std::pair<std::string, std::wstring>("tian", L"天田添填甜舔恬腆掭钿阗忝殄畋锘"));
  mapcode.insert(std::pair<std::string, std::wstring>("tiao", L"条跳挑调迢眺龆笤祧蜩髫佻窕鲦苕粜铫"));
  mapcode.insert(std::pair<std::string, std::wstring>("tie", L"铁贴帖萜餮锇"));
  mapcode.insert(std::pair<std::string, std::wstring>("ting", L"听停挺厅亭艇庭廷烃汀莛铤葶婷蜓梃霆"));
  mapcode.insert(std::pair<std::string, std::wstring>("tong", L"同通痛铜桶筒捅统童彤桐瞳酮潼茼仝砼峒恸佟嗵垌僮"));
  mapcode.insert(std::pair<std::string, std::wstring>("tou", L"头偷透投钭骰亠"));
  mapcode.insert(std::pair<std::string, std::wstring>("tu", L"土图兔涂吐秃突徒凸途屠酴荼钍菟堍"));
  mapcode.insert(std::pair<std::string, std::wstring>("tuan", L"团湍疃抟彖"));
  mapcode.insert(std::pair<std::string, std::wstring>("tui", L"腿推退褪颓蜕煺"));
  mapcode.insert(std::pair<std::string, std::wstring>("tun", L"吞屯褪臀囤氽饨豚暾"));
  mapcode.insert(std::pair<std::string, std::wstring>("tuo", L"拖脱托妥驮拓驼椭唾鸵陀橐柝跎乇坨佗庹酡柁鼍沱箨砣说铊"));
  mapcode.insert(std::pair<std::string, std::wstring>("wa", L"挖瓦蛙哇娃洼袜佤娲腽"));
  mapcode.insert(std::pair<std::string, std::wstring>("wai", L"外歪崴"));
  mapcode.insert(std::pair<std::string, std::wstring>("wan", L"完万晚碗玩弯挽湾丸腕宛婉烷顽豌惋皖蔓莞脘蜿绾芄琬纨剜畹菀"));
  mapcode.insert(std::pair<std::string, std::wstring>("wang", L"望忘王往网亡枉旺汪妄辋魍惘罔尢"));
  mapcode.insert(std::pair<std::string, std::wstring>("wei", L"为位未围喂胃微味尾伪威伟卫危违委魏唯维畏惟韦巍蔚谓尉潍纬慰桅萎苇渭葳帏艉鲔娓逶闱隈沩玮涠帷崴隗诿洧偎猥猬嵬軎韪炜煨圩薇痿囗"));
  mapcode.insert(std::pair<std::string, std::wstring>("wen", L"问文闻稳温吻蚊纹瘟紊汶阌刎雯璺"));
  mapcode.insert(std::pair<std::string, std::wstring>("weng", L"翁嗡瓮蕹蓊"));
  mapcode.insert(std::pair<std::string, std::wstring>("wo", L"我握窝卧挝沃蜗涡斡倭幄龌肟莴喔渥硪"));
  mapcode.insert(std::pair<std::string, std::wstring>("wu", L"无五屋物舞雾误捂污悟勿钨武戊务呜伍吴午吾侮乌毋恶诬芜巫晤梧坞妩蜈牾寤兀怃阢邬唔忤骛於鋈仵杌鹜婺迕痦芴焐庑鹉鼯浯圬"));
  mapcode.insert(std::pair<std::string, std::wstring>("xi", L"西洗细吸戏系喜席稀溪熄锡膝息袭惜习嘻夕悉矽熙希檄牺晰昔媳硒铣烯析隙汐犀蜥奚浠葸饩屣玺嬉禊兮翕穸禧僖淅蓰舾蹊醯郗欷皙蟋羲茜徙隰唏曦螅歙樨阋粞熹觋菥鼷裼舄"));
  mapcode.insert(std::pair<std::string, std::wstring>("xia", L"下吓夏峡虾瞎霞狭匣侠辖厦暇狎柙呷黠硖罅遐瑕"));
  mapcode.insert(std::pair<std::string, std::wstring>("xian", L"先线县现显掀闲献嫌陷险鲜弦衔馅限咸锨仙腺贤纤宪舷涎羡铣苋藓岘痫莶籼娴蚬猃祆冼燹跣跹酰暹氙鹇筅霰洗"));
  mapcode.insert(std::pair<std::string, std::wstring>("xiang", L"想向象项响香乡相像箱巷享镶厢降翔祥橡详湘襄飨鲞骧蟓庠芗饷缃葙"));
  mapcode.insert(std::pair<std::string, std::wstring>("xiao", L"小笑消削销萧效宵晓肖孝硝淆啸霄哮嚣校魈蛸骁枵哓筱潇逍枭绡箫"));
  mapcode.insert(std::pair<std::string, std::wstring>("xie", L"写些鞋歇斜血谢卸挟屑蟹泻懈泄楔邪协械谐蝎携胁解叶绁颉缬獬榭廨撷偕瀣渫亵榍邂薤躞燮勰骱鲑"));
  mapcode.insert(std::pair<std::string, std::wstring>("xin", L"新心欣信芯薪锌辛衅忻歆囟莘镡馨鑫昕忄"));
  mapcode.insert(std::pair<std::string, std::wstring>("xing", L"性行型形星醒姓腥刑杏兴幸邢猩惺省硎悻荥陉擤荇研饧"));
  mapcode.insert(std::pair<std::string, std::wstring>("xiong", L"胸雄凶兄熊汹匈芎"));
  mapcode.insert(std::pair<std::string, std::wstring>("xiu", L"修锈绣休羞宿嗅袖秀朽臭溴貅馐髹鸺咻庥岫"));
  mapcode.insert(std::pair<std::string, std::wstring>("xu", L"许须需虚嘘蓄续序叙畜絮婿戌徐旭绪吁酗恤墟糈勖栩浒蓿顼圩洫胥醑诩溆煦盱"));
  mapcode.insert(std::pair<std::string, std::wstring>("xuan", L"选悬旋玄宣喧轩绚眩癣券暄楦儇渲漩泫铉璇煊碹镟炫揎萱谖"));
  mapcode.insert(std::pair<std::string, std::wstring>("xue", L"学雪血靴穴削薛踅噱鳕泶谑"));
  mapcode.insert(std::pair<std::string, std::wstring>("xun", L"寻讯熏训循殉旬巡迅驯汛逊勋询浚巽鲟浔埙恂獯醺洵郇峋蕈薰荀窨曛徇荨"));
  mapcode.insert(std::pair<std::string, std::wstring>("ya", L"呀压牙押芽鸭轧崖哑亚涯丫雅衙鸦讶蚜垭疋砑琊桠睚娅痖岈氩伢迓揠"));
  mapcode.insert(std::pair<std::string, std::wstring>("yan", L"眼烟沿盐言演严咽淹炎掩厌宴岩研延堰验艳殷阉砚雁唁彦焰蜒衍谚燕颜阎铅焉奄芫厣阏菸魇琰滟焱赝筵腌兖剡餍恹罨檐湮偃谳胭晏闫俨郾酽鄢妍鼹崦阽嫣涎讠"));
  mapcode.insert(std::pair<std::string, std::wstring>("yang", L"样养羊洋仰扬秧氧痒杨漾阳殃央鸯佯疡炀恙徉鞅泱蛘烊怏"));
  mapcode.insert(std::pair<std::string, std::wstring>("yao", L"要摇药咬腰窑舀邀妖谣遥姚瑶耀尧钥侥疟珧夭鳐鹞轺爻吆铫幺崾肴曜徭杳窈啮繇"));
  mapcode.insert(std::pair<std::string, std::wstring>("ye", L"也夜业野叶爷页液掖腋冶噎耶咽曳椰邪谒邺晔烨揶铘靥"));
  mapcode.insert(std::pair<std::string, std::wstring>("yi", L"一以已亿衣移依易医乙仪亦椅益倚姨翼译伊遗艾胰疑沂宜异彝壹蚁谊揖铱矣翌艺抑绎邑屹尾役臆逸肄疫颐裔意毅忆义夷溢诣议怿痍镒癔怡驿旖熠酏翊欹峄圯殪咦懿噫劓诒饴漪佚咿瘗猗眙羿弈苡荑佾贻钇缢迤刈悒黟翳弋奕蜴埸挹嶷薏呓轶镱舣奇硪衤铊"));
  mapcode.insert(std::pair<std::string, std::wstring>("yin", L"因引印银音饮阴隐荫吟尹寅茵淫殷姻堙鄞喑夤胤龈吲狺垠霪蚓氤铟窨瘾洇茚廴"));
  mapcode.insert(std::pair<std::string, std::wstring>("ying", L"应硬影营迎映蝇赢鹰英颖莹盈婴樱缨荧萤萦楹蓥瘿茔鹦媵莺璎郢嘤撄瑛滢潆嬴罂瀛膺荥颍"));
  mapcode.insert(std::pair<std::string, std::wstring>("yo", L"哟育唷"));
  mapcode.insert(std::pair<std::string, std::wstring>("yong", L"用涌永拥蛹勇雍咏泳佣踊痈庸臃恿壅慵俑墉鳙邕喁甬饔镛"));
  mapcode.insert(std::pair<std::string, std::wstring>("you", L"有又由右油游幼优友铀忧尤犹诱悠邮酉佑釉幽疣攸蚰莠鱿卣黝莸猷蚴宥牖囿柚蝣莜鼬铕蝤繇呦侑尢"));
  mapcode.insert(std::pair<std::string, std::wstring>("yu", L"与于欲鱼雨余遇语愈狱玉渔予誉育愚羽虞娱淤舆屿禹宇迂俞逾域芋郁吁盂喻峪御愉渝尉榆隅浴寓裕预豫驭蔚妪嵛雩馀阈窬鹆妤揄窳觎臾舁龉蓣煜钰谀纡於竽瑜禺聿欤俣伛圄鹬庾昱萸瘐谕鬻圉瘀熨饫毓燠腴狳菀蜮蝓吾"));
  mapcode.insert(std::pair<std::string, std::wstring>("yuan", L"远员元院圆原愿园援猿怨冤源缘袁渊苑垣鸳辕圜鼋橼媛爰眢鸢掾芫沅瑗螈箢塬"));
  mapcode.insert(std::pair<std::string, std::wstring>("yue", L"月越约跃阅乐岳悦曰说粤钥瀹钺刖龠栎樾哕"));
  mapcode.insert(std::pair<std::string, std::wstring>("yun", L"云运晕允匀韵陨孕耘蕴酝郧员氲恽愠郓芸筠韫昀狁殒纭熨"));
  mapcode.insert(std::pair<std::string, std::wstring>("za", L"杂砸咋匝扎咂拶"));
  mapcode.insert(std::pair<std::string, std::wstring>("zai", L"在再灾载栽宰哉甾崽"));
  mapcode.insert(std::pair<std::string, std::wstring>("zan", L"咱暂攒赞簪趱糌瓒拶昝錾"));
  mapcode.insert(std::pair<std::string, std::wstring>("zang", L"脏葬赃藏臧驵"));
  mapcode.insert(std::pair<std::string, std::wstring>("zao", L"早造遭糟灶燥枣凿躁藻皂噪澡蚤唣"));
  mapcode.insert(std::pair<std::string, std::wstring>("ze", L"则责择泽咋箦舴帻迮啧仄昃笮赜"));
  mapcode.insert(std::pair<std::string, std::wstring>("zei", L"贼"));
  mapcode.insert(std::pair<std::string, std::wstring>("zen", L"怎谮"));
  mapcode.insert(std::pair<std::string, std::wstring>("zeng", L"增赠憎曾缯罾甑锃"));
  mapcode.insert(std::pair<std::string, std::wstring>("zha", L"扎炸渣闸眨榨乍轧诈铡札查栅咋喳砟痄吒哳楂蚱揸喋柞咤齄龃"));
  mapcode.insert(std::pair<std::string, std::wstring>("zhai", L"摘窄债斋寨择翟宅砦瘵"));
  mapcode.insert(std::pair<std::string, std::wstring>("zhan", L"站占战盏沾粘毡展栈詹颤蘸湛绽斩辗崭瞻谵搌旃骣"));
  mapcode.insert(std::pair<std::string, std::wstring>("zhang", L"张章长帐仗丈掌涨账樟杖彰漳胀瘴障仉嫜幛鄣璋嶂獐蟑"));
  mapcode.insert(std::pair<std::string, std::wstring>("zhao", L"找着照招罩爪兆朝昭沼肇召赵棹啁钊笊诏"));
  mapcode.insert(std::pair<std::string, std::wstring>("zhe", L"着这者折遮蛰哲蔗锗辙浙柘辄赭摺鹧磔褶蜇谪"));
  mapcode.insert(std::pair<std::string, std::wstring>("zhen", L"真阵镇针震枕振斟珍疹诊甄砧臻贞侦缜蓁祯箴轸榛稹赈朕鸩胗浈桢畛圳椹溱"));
  mapcode.insert(std::pair<std::string, std::wstring>("zheng", L"正整睁争挣征怔证症郑拯蒸狰政峥钲铮筝诤徵鲭"));
  mapcode.insert(std::pair<std::string, std::wstring>("zhi", L"只之直知制指纸支芝枝稚吱蜘质肢脂汁炙织职痔植抵殖执值侄址滞止趾治旨窒志挚掷至致置帜识峙智秩帙摭黹桎枳轵忮祉蛭膣觯郅栀彘芷祗咫鸷絷踬胝骘轾痣陟踯雉埴贽卮酯豸跖栉夂徵"));
  mapcode.insert(std::pair<std::string, std::wstring>("zhong", L"中重种钟肿众终盅忠仲衷踵舯螽锺冢忪"));
  mapcode.insert(std::pair<std::string, std::wstring>("zhou", L"周洲皱粥州轴舟昼骤宙诌肘帚咒繇胄纣荮啁碡绉籀妯酎"));
  mapcode.insert(std::pair<std::string, std::wstring>("zhu", L"住主猪竹株煮筑著贮铸嘱拄注祝驻属术珠瞩蛛朱柱诸诛逐助烛蛀潴洙伫瘃翥茱苎橥舳杼箸炷侏铢疰渚褚躅麈邾槠竺丶"));
  mapcode.insert(std::pair<std::string, std::wstring>("zhua", L"抓爪挝"));
  mapcode.insert(std::pair<std::string, std::wstring>("zhuai", L"拽转"));
  mapcode.insert(std::pair<std::string, std::wstring>("zhuan", L"转专砖赚传撰篆颛馔啭沌"));
  mapcode.insert(std::pair<std::string, std::wstring>("zhuang", L"装撞庄壮桩状幢妆奘戆"));
  mapcode.insert(std::pair<std::string, std::wstring>("zhui", L"追坠缀锥赘椎骓惴缒隹"));
  mapcode.insert(std::pair<std::string, std::wstring>("zhun", L"准谆肫窀饨"));
  mapcode.insert(std::pair<std::string, std::wstring>("zhuo", L"捉桌着啄拙灼浊卓琢茁酌擢焯濯诼浞涿倬镯禚斫淖"));
  mapcode.insert(std::pair<std::string, std::wstring>("zi", L"字自子紫籽资姿吱滓仔兹咨孜渍滋淄笫粢龇秭恣谘趑缁梓鲻锱孳耔觜髭赀茈訾嵫眦姊辎"));
  mapcode.insert(std::pair<std::string, std::wstring>("zong", L"总纵宗棕综踪鬃偬粽枞腙"));
  mapcode.insert(std::pair<std::string, std::wstring>("zou", L"走揍奏邹鲰鄹陬驺诹"));
  mapcode.insert(std::pair<std::string, std::wstring>("zu", L"组族足阻租祖诅菹镞卒俎"));
  mapcode.insert(std::pair<std::string, std::wstring>("zuan", L"钻纂缵躜攥"));
  mapcode.insert(std::pair<std::string, std::wstring>("zui", L"最嘴醉罪觜蕞"));
  mapcode.insert(std::pair<std::string, std::wstring>("zun", L"尊遵鳟撙樽"));
  mapcode.insert(std::pair<std::string, std::wstring>("zuo", L"做作坐左座昨琢撮佐嘬酢唑祚胙怍阼柞砟"));

  return mapcode;
};
static std::map<std::string, std::wstring> codemap = fillCodemap();

CInputCodingTableBasePY::CInputCodingTableBasePY()
{
  m_codechars = "abcdefghijklmnopqrstuvwxyz";
}

std::vector<std::wstring> CInputCodingTableBasePY::GetResponse(int)
{
  return m_words;
}

bool CInputCodingTableBasePY::GetWordListPage(const std::string& strCode, bool isFirstPage)
{
  if (!isFirstPage)
    return false;

  m_words.clear();
  std::map<std::string, std::wstring>::iterator finder = codemap.find(strCode);
  if (finder != codemap.end())
  {
    for (unsigned int i = 0; i < finder->second.size(); i++)
    {
      m_words.push_back(finder->second.substr(i, 1));
    }
  }
  CGUIMessage msg(GUI_MSG_CODINGTABLE_LOOKUP_COMPLETED, 0, 0, 0);
  msg.SetStringParam(strCode);
  g_windowManager.SendThreadMessage(msg, g_windowManager.GetActiveWindowID());
  return true;
}
