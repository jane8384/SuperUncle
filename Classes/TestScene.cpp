#include <string>
#include <vector>
#include <iostream>
#include "TestScene.h"
#include "cocostudio/CocoStudio.h"
#include "ui/CocosGUI.h"
#include "SimpleAudioEngine.h"
#include "Character.h"
#include "Controler.h"
#include "VirtualRockerAndButton.h"
#include "ContactListener.h"


USING_NS_CC;

using namespace std;
using namespace CocosDenshion;


extern vector<b2Body *> vector_MapBody;  //刚体容器

Layer* Layer_BG;             //背景+云
Layer* Layer_UI;             //金币+得分+时间
Layer* Layer_Control;        //摇杆+按钮
Layer* Layer_GameSettings;   //游戏相关设置
Layer* Layer_TitledMap;      //瓦片地图


TMXTiledMap *tiledMap;       //瓦片地图

extern Size visSize;


void TestScene::initPysics() //初始物理引擎
{
	b2Vec2 gravity;
	gravity.Set(0.0f, -50.0f);
	world = new b2World(gravity);

	world->SetAllowSleeping(false);  //否则穿透后变为静态刚体

	world->SetContinuousPhysics(true);

	b2BodyDef groundBodyDef;
	groundBodyDef.position.Set(0, 0); // bottom-left corner


	b2Body* groundBody = world->CreateBody(&groundBodyDef);

	// Define the ground box shape.
	b2EdgeShape groundBox;

	// bottom
	groundBox.Set(b2Vec2(0 / PTM_RATIO, 0 / PTM_RATIO), b2Vec2(visSize.width / PTM_RATIO, 0 / PTM_RATIO));
	groundBody->CreateFixture(&groundBox, 0);

	// top
	groundBox.Set(b2Vec2(0 / PTM_RATIO, visSize.height * 2 / PTM_RATIO), b2Vec2(visSize.width / PTM_RATIO, visSize.height * 2 / PTM_RATIO));
	groundBody->CreateFixture(&groundBox, 0);

	// left
	groundBox.Set(b2Vec2(0 / PTM_RATIO, 0 / PTM_RATIO), b2Vec2(0 / PTM_RATIO, visSize.height * 2 / PTM_RATIO));
	groundBody->CreateFixture(&groundBox, 0);

	// right
	groundBox.Set(b2Vec2(visSize.width / PTM_RATIO, 0), b2Vec2(visSize.width / PTM_RATIO, visSize.height * 2 / PTM_RATIO));
	groundBody->CreateFixture(&groundBox, 0);


	//遮罩
	_debugDraw = new GLESDebugDraw(32);

	uint32 flags = 0;
	flags += b2Draw::e_shapeBit;
	//flags += b2Draw::e_jointBit;
	//flags += b2Draw::e_aabbBit;
	//flags += b2Draw::e_pairBit;
	//flags += b2Draw::e_centerOfMassBit;

	_debugDraw->SetFlags(flags);
	world->SetDebugDraw(_debugDraw);

	//显示遮罩需屏蔽层
	Layer_BG->setVisible(false);
	Layer_Control->setVisible(false);
	Layer_TitledMap->setVisible(false);

	//设置碰撞监听事件
	contLis = new ContactListener();
	world->SetContactListener(contLis);
}

void TestScene::createPhysicalUnCross()  //根据瓦片地图创建相应刚体
{
	//获取所有层
	auto Vec_objGroups = tiledMap->getObjectGroups();

	for (auto i : Vec_objGroups)
	{
		for (auto j : i->getObjects())
		{
			float obj_width = j.asValueMap().at("width").asFloat();
			float obj_height = j.asValueMap().at("height").asFloat();

			/*这里是个巨坑：取到的瓦片地图中对象的坐标是该对象锚点为（0,0）时的坐标，而box2D在场景中绘制刚体时，会将刚才得到的坐标以锚点为（0.5, 0.5）进行绘制，
			且无法改变刚体锚点，所以，需要将得到的坐标转为锚点是（0.5, 0.5）时的坐标，故将坐标的x,y，各自挪动其长度的一半，即可完成坐标转换，使得刚体与瓦片重合*/

			float obj_X = j.asValueMap().at("x").asFloat() + obj_width / 2;
			float obj_Y = j.asValueMap().at("y").asFloat() + obj_height / 2;


			CCLOG("Run to createPhysicalUnCross x:%f, y:%f, width: %f, height: %f", obj_X, obj_Y, obj_width, obj_height);


			b2BodyDef body_def;
			body_def.type = b2_staticBody;
			body_def.position.Set(obj_X / PTM_RATIO, obj_Y / PTM_RATIO);
			auto _pyhsicalBody = world->CreateBody(&body_def);


			b2PolygonShape polygon;
			polygon.SetAsBox(obj_width / PTM_RATIO / 2, obj_height / PTM_RATIO / 2);
			b2FixtureDef fix_def;
			fix_def.restitution = 0.f;
			fix_def.shape = &polygon;

			_pyhsicalBody->CreateFixture(&fix_def);

			vector_MapBody.push_back(_pyhsicalBody); //将地图产生刚体添加至容器
		}
	}
}

void TestScene::draw(Renderer * renderer, const Mat4 & transform, uint32_t flags)  //刚体调试=>遮罩
{
	GL::enableVertexAttribs(cocos2d::GL::VERTEX_ATTRIB_FLAG_POSITION);

	world->DrawDebugData();

	CHECK_GL_ERROR_DEBUG();
}

Scene* TestScene::createScene()
{
	auto scene = Scene::create();
	auto layer = TestScene::create();
	scene->addChild(layer);
	return scene;
}

bool TestScene::init()
{
	if (!Layer::init())
	{
		return false;
	}

	visSize = Director::getInstance()->getVisibleSize();


	Layer_BG = Layer::create();
	Layer_BG->setName("Layer_BG");
	this->addChild(Layer_BG, 0);

	Layer_UI = Layer::create();
	Layer_UI->setName("Layer_UI");
	this->addChild(Layer_UI, 10);

	Layer_Control = Layer::create();
	Layer_Control->setName("Layer_Control");
	this->addChild(Layer_Control, 11);

	Layer_GameSettings = Layer::create();
	Layer_GameSettings->setName("Layer_GameSettings");
	this->addChild(Layer_GameSettings, 12);

	Layer_TitledMap = Layer::create();
	Layer_TitledMap->setName("Layer_TitledMap");
	this->addChild(Layer_TitledMap, 1);

	tiledMap = cocos2d::TMXTiledMap::create("MAP/Mission1.tmx");

	initPysics();
	createPhysicalUnCross(); //根据瓦片地图创建不可通行层

	//背景图片
	Controler::createBackGround();

	//虚拟摇杆及事件
	VirtualRockerAndButton::getInstance();

	//抗锯齿
	auto pChildrenArray = tiledMap->getChildren();

	SpriteBatchNode* child = NULL;
	Ref* pObject = NULL;

	for (auto it = pChildrenArray.begin(); it != pChildrenArray.end(); it++) {
		pObject = *it;
		child = (SpriteBatchNode*)pObject;
		child->getTexture()->setAliasTexParameters();
	}

	//正交视图
	Director::getInstance()->setProjection(Director::Projection::_2D);

	//角色初始化相关
	Character::getInstance()->initBeginPos();
	Character::getInstance()->sp_man->setGlobalZOrder(1);
	this->addChild(Character::getInstance()->sp_man);

	Character::getInstance()->sp_man->setAnchorPoint(Vec2(0, 1.1));  //角色与刚体吻合调整
	b2BodyDef bodydef;
	bodydef.type = b2_dynamicBody;
	bodydef.position.Set(Character::getInstance()->sp_man->getPosition().x / PTM_RATIO, Character::getInstance()->sp_man->getPosition().y / PTM_RATIO);

	Character::getInstance()->body_man = world->CreateBody(&bodydef);
	Character::getInstance()->body_man->SetUserData(Character::getInstance()->sp_man);

	//角色刚体信息
	b2Vec2 points[4] = {
		b2Vec2(-20.0000 / PTM_RATIO, -46.00000 / PTM_RATIO),
		b2Vec2(-26.50000 / PTM_RATIO, 46.00000 / PTM_RATIO),
		b2Vec2(26.50000 / PTM_RATIO, 46.00000 / PTM_RATIO),
		b2Vec2(20.0000 / PTM_RATIO, -46.00000 / PTM_RATIO)
	};

	b2PolygonShape shape_body_man;
	shape_body_man.Set(points, 4);

	b2FixtureDef fixturedef;
	fixturedef.density = 1.0f;    //密度
	fixturedef.friction = 0.2f;   //摩擦
	fixturedef.restitution = 0.f; //弹性
	fixturedef.shape = &shape_body_man;
	Character::getInstance()->body_man->CreateFixture(&fixturedef);
	Character::getInstance()->body_man->SetFixedRotation(true);  //刚体不模拟旋转


	//加载瓦片地图
	Layer_TitledMap->addChild(tiledMap);

	//BGM
	SimpleAudioEngine::getInstance()->playBackgroundMusic("res/BGM/Mission1BGM.mp3", true);

	//定时器
	this->schedule(CC_CALLBACK_1(TestScene::update_per_second, this), 1.0f, "oneSecond");    //游戏时间衰减，每1.0秒后调用
	this->scheduleUpdate();

	//重力计监听事件 => 摇一摇暂停游戏
	auto eventListenerAcceleration = EventListenerAcceleration::create(CC_CALLBACK_2(Controler::onAcceleration, Controler::getInstance()));
	_eventDispatcher->addEventListenerWithSceneGraphPriority(eventListenerAcceleration, this);
	Device::setAccelerometerEnabled(true); //设备开启重力计

	//键盘监听事件
	auto eventListenerKeyboard = EventListenerKeyboard::create();
	eventListenerKeyboard->onKeyPressed = CC_CALLBACK_2(Controler::onKeyPressed, Controler::getInstance());
	eventListenerKeyboard->onKeyReleased = CC_CALLBACK_2(Controler::onKeyReleased, Controler::getInstance());
	_eventDispatcher->addEventListenerWithSceneGraphPriority(eventListenerKeyboard, this);

	//监听遥感按钮等触摸事件
	auto eventListenerTouch = EventListenerTouchAllAtOnce::create();
	eventListenerTouch->onTouchesBegan = CC_CALLBACK_2(VirtualRockerAndButton::onTouchesBegan, VirtualRockerAndButton::getInstance());
	eventListenerTouch->onTouchesMoved = CC_CALLBACK_2(VirtualRockerAndButton::onTouchesMoved, VirtualRockerAndButton::getInstance());
	eventListenerTouch->onTouchesEnded = CC_CALLBACK_2(VirtualRockerAndButton::onTouchesEnded, VirtualRockerAndButton::getInstance());
	_eventDispatcher->addEventListenerWithSceneGraphPriority(eventListenerTouch, this);

	return true;
}

void TestScene::update_per_second(float delta)
{
	Controler::CreateUpdateUI(); //创建|刷新时间等UI  =>   后期得分方式改为：观察者模式
	Controler::createCloud();
}

void TestScene::update(float delta)
{
	//物理模拟后期放入Controler类
	world->Step(delta, 10, 10);
	for (auto body = world->GetBodyList(); body != nullptr; body = body->GetNext())
	{
		if (body->GetUserData() != nullptr)
		{
			auto sprite = (Sprite *)body->GetUserData();
			sprite->setPosition(Vec2(body->GetPosition().x * PTM_RATIO, body->GetPosition().y * PTM_RATIO));
			//sprite->setRotation(-1 * CC_RADIANS_TO_DEGREES(body->GetAngle())); //不模拟旋转
		}
	}

	Controler::tiledMapScroll(delta, world);      //地图滚动
	Controler::cloudPosControl();                 //云朵位置控制
	Controler::keyBoardControler(delta);          //键盘控制器进一步处理  =>  触控也调用
	VirtualRockerAndButton::touchMoveControl();   //触摸行走控制
}

TestScene::TestScene()
{
}

TestScene::~TestScene()
{
	CC_SAFE_DELETE(world);
	CC_SAFE_DELETE(contLis);
}
