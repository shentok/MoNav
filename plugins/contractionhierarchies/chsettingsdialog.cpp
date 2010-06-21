#include "chsettingsdialog.h"
#include "ui_chsettingsdialog.h"
#include <QSettings>
#include <omp.h>

CHSettingsDialog::CHSettingsDialog(QWidget *parent) :
    QDialog(parent),
	 ui(new Ui::CHSettingsDialog)
{
    ui->setupUi(this);

	QSettings settings( "MoNav" );
	settings.beginGroup( "Contraction Hierarchies" );
	ui->blockSize->setValue( settings.value( "blockSize", 12 ).toInt() );
	ui->threads->setValue( omp_get_max_threads() );
	ui->threads->setMaximum( omp_get_max_threads() );
}

CHSettingsDialog::~CHSettingsDialog()
{
	QSettings settings( "MoNav" );
	settings.beginGroup( "Contraction Hierarchies" );
	settings.setValue( "blockSize", ui->blockSize->value() );

    delete ui;
}

void CHSettingsDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

bool CHSettingsDialog::getSettings( Settings* settings )
{
	if ( settings == NULL )
		return false;
	settings->blockSize = ui->blockSize->value();
	settings->threads = ui->threads->value();
	return true;
}
