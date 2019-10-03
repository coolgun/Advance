#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QTableWidget>
#include <QDate>
#include <QDialog>


struct CardIndexDescr
{
	int			id;
	QString		name;
	QDateTime	date;
	QColor		color;
};

//class QTreeWidget;




class CardIndex : public QWidget
{
	Q_OBJECT
		CardIndexDescr descr;
		class QLabel *name{};
		class QLabel *date{};
		class QLabel *forgot{};
		class QLabel *forgot_img{};
		class QLabel *doubt{};
		class QLabel *doubt_img{};

	public:
		explicit CardIndex(const CardIndexDescr& descr);
		void updateView();
signals:
	void delete_me(int);



};


class Advance : public QMainWindow
{
	void update_cards();
	QTableWidget *main_table{};
	
public:
	Advance(QWidget *parent = Q_NULLPTR);
	void addCardIndex();
	void delCardIndex(uint id);

private:
	
};
