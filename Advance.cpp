#include "Advance.h"
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QTextToSpeech>
#include <QInputDialog>
#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QGestureEvent>
#include <QToolButton>
#include <QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QUrl>
#include <QTextCodec> 
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QEventLoop>
#include <QElapsedTimer>
#include <random>

QNetworkAccessManager *manager{};
QString main_url("http://coolgun.tigraha.com/coolgun/index.php");

extern QString user_name;

static std::unique_ptr<QPixmap>  red_pix;
static std::unique_ptr<QPixmap>  green_pix;
static std::unique_ptr<QPixmap>  yellow_pix;
static QTextToSpeech   *speech{};

static QIcon red_box;
static QIcon yellow_box;
static QIcon green_box;

/*

class CardIndex : public QWidget
{
		CardIndexDescr descr;
		class QLabel *name;
		class QLabel *date;

	public:

		explicit CardIndex(const CardIndexDescr& descr);
		void updateView();
		void edit();
		void del();
		void changeWordList();
};
*/

QJsonArray   query(const QString  &text = {})
{
	QJsonArray  ret;
	QNetworkRequest request(main_url+text);
	QNetworkReply *reply = manager->get(request);

	QEventLoop loop;
//	QTimer::singleShot(500, reply, SLOT(abort()));   // не уверен что оно нужно в моих условиях.
	QObject::connect(reply, &QNetworkReply::finished, &loop,&QEventLoop::quit);
	loop.exec();

	if (reply->error() == QNetworkReply::NoError) 
	{
		const auto content = reply->readAll();
		ret = QJsonDocument::fromJson(content).array();
	}
	
	reply->close();
	return ret;
}



class TwoWordsDlg : public QDialog
{
	
public:
	QLineEdit  *rusWord;
	QLineEdit  *engWord;


	TwoWordsDlg(QWidget *parent = {}) :QDialog(parent)
	{
		
		auto font = this->font();
		font.setPointSize(20);
		setFont(font);

		auto *l = new QGridLayout;
		l->setContentsMargins(1, 1, 1, 1);

		
		rusWord = new QLineEdit;
		engWord = new QLineEdit;
		l->addWidget(new QLabel("Eng"), 0, 0); l->addWidget(engWord, 0, 1);
		l->addWidget(new QLabel("Rus"), 1, 0); l->addWidget(rusWord, 1, 1);
		auto *btn = new QPushButton("Speech");
		connect(btn, &QPushButton::pressed, [this]
		{
			speech->say(engWord->text());
		}
		);

		l->addWidget(btn, 2, 0, 1, 2);
		auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		l->addWidget(buttonBox, 3, 0, 1, 2);

		setLayout(l);

		

		connect(buttonBox, &QDialogButtonBox::accepted, [this]
		{
			if (rusWord->text().isEmpty() || engWord->text().isEmpty())
			{
				QMessageBox::information(this, "Advance", "Input Words");
				if (engWord->text().isEmpty())
					engWord->setFocus();
				else
					rusWord->setFocus();
				return;
			}

			accept();
		}

		);
		connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	}


};

class WordTestDlg : public QDialog
{
	QList<QPair<QString, QString>> words;

	int doubt_count{};
	int forgot_count{};
	qint64 start{};
	QElapsedTimer el_timer;
	QDialogButtonBox *buttonBox{};

	QLabel *main_label{};
	QLabel *second_label{};
	QPushButton *speech_btn{};

	void save(uint id)
	{
	
		query(QString("?InsTestCardId=%1&start=%2&TimePass=%3&Forgot=%4&Doubt=%5&User=%6")
			.arg(id)
			.arg(start)
			.arg(el_timer.elapsed())
			.arg(forgot_count)
			.arg(doubt_count)
			.arg(user_name)
		);

	}

	bool event(QEvent *event) override
	{
		if (event->type() == QEvent::Gesture)
		{
			const auto *gesture_event = static_cast<QGestureEvent*>(event);
			const auto *gesture = gesture_event->gesture(Qt::SwipeGesture);
			if (gesture)
			{
				const auto *swipe = static_cast<const QSwipeGesture*>(gesture);
				if (swipe->state() == Qt::GestureFinished) 
				{
					if (swipe->horizontalDirection() == QSwipeGesture::Left|| swipe->horizontalDirection() == QSwipeGesture::Right)
					{
						nextWord();
					}
				}
			}
		}

		return QWidget::event(event);
	}

	uint id;

	void nextWord()
	{
		speech_btn->hide();
		words.pop_front();
		if (words.empty())
		{
			save(id);
			accept();
			return;
		}

		buttonBox->button(QDialogButtonBox::Close)->setDisabled(false);
		buttonBox->button(QDialogButtonBox::Open)->setDisabled(false);
		main_label->setText(words.first().first);
		second_label->clear();
	}
public:
	WordTestDlg(uint id, bool is_eng, QWidget *parent = {}) :QDialog(parent),id(id)
	{

		grabGesture(Qt::SwipeGesture);
		
		for (const auto &value : query(QString("?WordCardIndexId=%1").arg(id)))
		{
			const auto query = value.toObject();
			auto p = qMakePair(query.value("EngWord").toString(), query.value("RusWord").toString());
			if (!is_eng)
				qSwap(p.first, p.second);
			words.push_back(p);
		}

        const auto seed = std::chrono::system_clock::now().time_since_epoch().count();
		std::shuffle(words.begin(), words.end(), std::default_random_engine(seed));

		auto  *h = new QHBoxLayout;
		auto  *l = new QVBoxLayout;
		auto *time_label = new QLabel;
		h->addWidget(time_label);
		auto *close_button = new QPushButton("Close");
		h->addWidget(close_button);
		connect(close_button, &QPushButton::pressed, this, &QDialog::reject);

		l->addLayout(h);
		main_label = new QLabel;
		l->setSizeConstraint(QLayout::SetMinimumSize);
		main_label->setWordWrap(true);
		main_label->setAlignment(Qt::AlignCenter);
		auto font = main_label->font();
		font.setPixelSize(80);
		main_label->setFont(font);

		second_label = new QLabel;
		second_label->setWordWrap(true);
		second_label->setAlignment(Qt::AlignCenter);
		font = second_label->font();
		font.setPixelSize(50);
		second_label->setFont(font);

		QPalette palette = second_label->palette();
		palette.setColor(second_label->foregroundRole(), Qt::red);
		second_label->setPalette(palette);

		speech_btn = new QPushButton("Speech");
		speech_btn->hide();
		connect(speech_btn, &QPushButton::pressed, [this, is_eng]
		{
			speech->say(is_eng? words.first().first : words.first().second);
		}
		);


		buttonBox = new QDialogButtonBox(QDialogButtonBox::Close | QDialogButtonBox::Open | QDialogButtonBox::Yes);
		buttonBox->setFont(font);
		buttonBox->button(QDialogButtonBox::Yes)->setText("I remember");
		buttonBox->button(QDialogButtonBox::Yes)->setIcon(green_box);
		buttonBox->button(QDialogButtonBox::Open)->setText("Doubt");
		buttonBox->button(QDialogButtonBox::Open)->setIcon(yellow_box);
		buttonBox->button(QDialogButtonBox::Close)->setText("Forgot");
		buttonBox->button(QDialogButtonBox::Close)->setIcon(red_box);
		
		l->addWidget(main_label, 1, Qt::AlignCenter);
		l->addWidget(second_label, 1, Qt::AlignCenter);
		l->addWidget(speech_btn, 0, Qt::AlignCenter);
		l->addWidget(buttonBox, 0, Qt::AlignCenter);
		l->addStretch(1);
		setLayout(l);



		const auto process = [this](QAbstractButton * button)
		{
			if (words.empty()) return;

			if (button == buttonBox->button(QDialogButtonBox::Yes))
			{
			
				nextWord();
			}
			else
			{
				second_label->setText(words.first().second);
				if (button == buttonBox->button(QDialogButtonBox::Open))
				{
					words.push_back(words.front());
					++doubt_count;
				}
				else
				{
					words.insert(words.size() / 2, words.front());
					++forgot_count;
				}
				buttonBox->button(QDialogButtonBox::Close)->setDisabled(true);
				buttonBox->button(QDialogButtonBox::Open)->setDisabled(true);
				speech_btn->show();
			}
		};

		start = QDateTime::currentSecsSinceEpoch();
		connect(buttonBox, &QDialogButtonBox::clicked, process);
		if (!words.empty())
		{
			main_label->setText(words.first().first);
		}

		auto *timer = new QTimer(this);
		connect(timer, &QTimer::timeout, [this, time_label]
		{
			time_label->setText(QString::number(el_timer.elapsed() / 1000));
		}
		);
		showFullScreen();
		timer->start(1000);
		el_timer.start();

	}

};

class WordListDlg : public QDialog
{
	uint id;
	QTableWidget *words;

	void refresh_table(uint id)
	{
		words->setRowCount(0);
		uint row{};
		for (const auto &value : query(QString("?WordCardIndexId=%1").arg(id)))
		{
			const auto query = value.toObject();
			words->setRowCount(row + 1);
			words->setItem(row, 0, new QTableWidgetItem(query.value("EngWord").toString(), query.value("Id").toInt()));
			words->setItem(row, 1, new QTableWidgetItem(query.value("RusWord").toString(), query.value("Id").toInt()));
			++row;
		}

		words->resizeRowsToContents();
	}

public:
	WordListDlg(uint id, QWidget *parent = {}) :QDialog(parent), id(id)
	{

		auto font = this->font();
		font.setPointSize(20);
		setFont(font);

		auto *l = new QVBoxLayout;
		l->setContentsMargins(1, 1, 1, 1);
		auto *tool_bar = new QToolBar;
		l->addWidget(tool_bar);
		auto *act = tool_bar->addAction("Add");
		connect(act, &QAction::triggered, [this]
		{
			TwoWordsDlg dlg(this);

			if(dlg.exec() > 0)
			{
				words->setRowCount(words->rowCount() + 1);
				
				query(QString("?InsWordCardId=%3&EngWord=%1&RusWord=%2")
					.arg(dlg.engWord->text())
					.arg(dlg.rusWord->text())
					.arg(this->id));

				refresh_table(this->id);
				words->selectRow(words->rowCount() - 1);
				dlg.engWord->clear();
				dlg.rusWord->clear();
				dlg.engWord->setFocus();
			}

		}
		);


		auto *edit_act = tool_bar->addAction("Edit");
		connect(edit_act, &QAction::triggered, [this]
		{
			const auto cur_row = words->currentRow();
			if (cur_row < 0) return;

			TwoWordsDlg dlg(this);

			dlg.engWord->setText(words->item(cur_row, 0)->text());
			dlg.rusWord->setText(words->item(cur_row, 1)->text());

			if (dlg.exec() > 0)
			{
				
				query(QString("?UpdWordCardId=%3&EngWord=%1&RusWord=%2")
						.arg(dlg.engWord->text())
						.arg(dlg.rusWord->text())
						.arg(words->item(cur_row, 0)->type())
				);
				

				words->item(cur_row, 0)->setText(dlg.engWord->text());
				words->item(cur_row, 1)->setText(dlg.rusWord->text());
			}
		}
		);

		act = tool_bar->addAction("Del");

		connect(act, &QAction::triggered, [this]
		{
			const auto cur_row = words->currentRow();
			if (cur_row < 0) return;

			query(QString("?DelWordCardId=%1").arg(words->item(cur_row, 0)->type()));

			words->removeRow(cur_row);
		}
		);


		act = tool_bar->addAction("Close");
		connect(act, &QAction::triggered, this,&QWidget::close);

		words = new QTableWidget(0, 2);
		font= words->font();
		font.setPointSize(20);
		words->setFont(font);

		connect(words, &QTableWidget::itemDoubleClicked, edit_act, &QAction::triggered);

		words->setHorizontalHeaderLabels({ "Eng","Rus" });
		words->setEditTriggers(QAbstractItemView::NoEditTriggers);
		words->setSelectionBehavior(QAbstractItemView::SelectRows);
		words->setSelectionMode(QAbstractItemView::SingleSelection);
		words->setAlternatingRowColors(true);
		l->addWidget(words);
		setLayout(l);

		refresh_table(id);
		showFullScreen();
		
		const int col_width = (words->width()) / 2 ;
		words->setColumnWidth(0, col_width);
		words->horizontalHeader()->setStretchLastSection(true);
	}


};



QString get_text(QWidget *parent, const QString &start)
{
	QString text;
	while (true)
	{
		bool ok{};
		text = QInputDialog::getText(parent, "Card Index Name", "name:", QLineEdit::Normal, start, &ok);
		if (!ok) return{};
		if (!text.isEmpty())
		{
			return text;
		}
	}
}




CardIndex::CardIndex(const CardIndexDescr& descr) :
	descr(descr)
{

	auto *tool_lan = new QToolBar;
	tool_lan->setOrientation(Qt::Vertical);
	tool_lan->setContentsMargins(1, 1, 1, 1);
	auto *act = tool_lan->addAction("rus");

	const auto show_test = [this](bool is_eng)
	{
		WordTestDlg dlg(this->descr.id, is_eng, this);
		dlg.exec();
		updateView();
	};

	connect(act, &QAction::triggered, std::bind(show_test, false));

	act = tool_lan->addAction("eng");
	connect(act, &QAction::triggered, std::bind(show_test, true));


	auto *tool = new QToolBar;
	tool->setOrientation(Qt::Vertical);
	tool->setContentsMargins(1, 1, 1, 1);
	act = tool->addAction("w");

	connect(act, &QAction::triggered, [this]
	{
		WordListDlg dlg(this->descr.id, this);
		dlg.exec();

	}
	);

	act = tool->addAction("e");

	connect(act, &QAction::triggered, [this]
	{
		const auto text = get_text(this, this->descr.name);
		if (text.isEmpty()) return;

		this->descr.name = text;

		query(QString("?InsCardIndexId=%2&Name=%1").arg(text).arg(this->descr.id));

		updateView();

	}
	);

	act = tool->addAction("x");

	connect(act, &QAction::triggered, [this]
	{
		emit delete_me(this->descr.id);
	}
	);


	auto *h = new QHBoxLayout;
	auto *h_local = new QHBoxLayout;
	auto *v = new QVBoxLayout;
	v->setContentsMargins(10, 0, 0, 0);
	h->setContentsMargins(1, 1, 1, 1);
	h->addWidget(tool_lan);
	h->addLayout(v);
	h->addWidget(tool);
	name = new QLabel;
	date = new QLabel;
	doubt_img = new QLabel;
	doubt = new QLabel;
	forgot_img = new QLabel;
	forgot = new QLabel;
	v->addWidget(name);
	doubt_img->setPixmap(*yellow_pix);
	forgot_img->setPixmap(*red_pix);
	h_local->addWidget(date);
	h_local->addWidget(doubt_img);
	h_local->addWidget(doubt);
	h_local->addWidget(forgot_img);
	h_local->addWidget(forgot);
	v->addLayout(h_local);
	setLayout(h);
	updateView();
	adjustSize();
	setMaximumHeight(height());
	setMinimumHeight(height());

}

void CardIndex::updateView()
{
	int cnt{};
	for (const auto &value : query(QString("?CntCardIndexId=%1").arg(descr.id)))
	{
		cnt = value.toObject().value("cnt").toInt();
	}

	name->setText(QString("<strong>%1</strong> - %3(%2)").arg(descr.name).arg(descr.date.toString("dd.MM.yyyy")).arg(cnt));

	date->clear();
	forgot->clear();
	doubt->clear();
	for (const auto &value : query(QString("?DscCardIndexId=%1&name=%2").arg(descr.id).arg(user_name)))
	{
		const auto query = value.toObject();
		date->setText(QString("<strong>%1</strong>(%2 sec)")
			.arg(QDateTime::fromSecsSinceEpoch(query.value("Date").toInt()).toString("dd.MM.yyyy"))
			.arg(query.value("TimePass").toInt() / 1000)
		);

		doubt->setText(QString::number(query.value("Doubt").toInt()));
		forgot->setText(QString::number(query.value("Forgot").toInt()));
	}


}


Advance::Advance(QWidget *parent)
	: QMainWindow(parent)

{
	
	manager = new QNetworkAccessManager(this);
	speech = new QTextToSpeech(this);
	red_pix = std::make_unique<QPixmap>(20, 20);
	red_pix->fill(Qt::red);
	red_box= QIcon(*red_pix);

	green_pix = std::make_unique<QPixmap>(20, 20);
	green_pix->fill(Qt::green);
	green_box = QIcon(*green_pix);

	yellow_pix = std::make_unique<QPixmap>(20, 20);
	yellow_pix->fill(Qt::yellow);
	yellow_box = QIcon(*yellow_pix);


	main_table = new QTableWidget(0, 1);
	main_table->verticalHeader()->hide();
	main_table->horizontalHeader()->hide();
	main_table->horizontalHeader()->setStretchLastSection(true);
	main_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	main_table->setSelectionMode(QAbstractItemView::NoSelection);

	setCentralWidget(main_table);

	auto * tool_bar = addToolBar("main");
	auto *act = tool_bar->addAction("Add Card Index");
	connect(act, &QAction::triggered, this, &Advance::addCardIndex);

	act = tool_bar->addAction("Close");
	connect(act, &QAction::triggered, this, &QWidget::close);
	update_cards();

}

void Advance::update_cards()
{
	main_table->setRowCount(0);
	uint row{};
	for (const auto &value : query())
	{
		const auto json_obj = value.toObject();
		const CardIndexDescr descr
		{
			
			json_obj.value("Id").toInt(),
			json_obj.value("Name").toString(),
			QDateTime::fromSecsSinceEpoch(json_obj.value("CreateDate").toInt()),
			QColor::fromRgba(json_obj.value("Color").toVariant().toULongLong())
		};
		auto *w = new CardIndex(descr);
		connect(w, &CardIndex::delete_me, this, &Advance::delCardIndex);
		main_table->setRowCount(row + 1);
		main_table->setRowHeight(row, w->height());
		main_table->setItem(row, 0, new QTableWidgetItem);
		main_table->setCellWidget(row, 0, w);
		main_table->item(row, 0)->setBackground(descr.color);
		++row;
	}

}

void Advance::delCardIndex(uint id)
{
	
	if (QMessageBox::question(this, "deletion card index", "Delete?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
	{

		query(QString("?DelCardIndexId=%1").arg(id));
		update_cards();
	}


}


void Advance::addCardIndex()
{
	const auto text = get_text(this, "Part number");

	if (text.isEmpty()) return;

	const auto colorNames = QColor::colorNames();

	query(QString("?InsCardIndexId=-1&Name=%1&CreateDate=%2&Color=%3")
		.arg(text)
		.arg(QDateTime::currentSecsSinceEpoch())
		.arg(QColor(colorNames[main_table->rowCount() % colorNames.size()]).rgb())
	);


	update_cards();
	main_table->selectRow(main_table->rowCount() - 1);


}
