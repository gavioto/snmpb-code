#include <QtGui>
#include <qfileinfo.h>
#include <qpainter.h>
#include "mibeditor.h"
#include "mibmodule.h"

MibEditor::MibEditor(Snmpb *snmpb)
{
    s = snmpb;

    // Menu items
    connect( s->MainUI()->fileNewAction, SIGNAL( triggered(bool) ),
             this, SLOT( MibFileNew(bool) ) );
    connect( s->MainUI()->fileOpenAction, SIGNAL( triggered(bool) ),
             this, SLOT( MibFileOpen(bool) ) );
    connect( s->MainUI()->fileSaveAction, SIGNAL( triggered(bool) ),
             this, SLOT( MibFileSave(bool) ) );
    connect( s->MainUI()->fileSaveAsAction, SIGNAL( triggered(bool) ),
             this, SLOT( MibFileSaveAs(bool) ) );
    connect( s->MainUI()->actionVerifyMIB, SIGNAL( triggered(bool) ),
             this, SLOT( VerifyMIB(bool) ) );
    connect( s->MainUI()->actionExtractMIBfromRFC, SIGNAL( triggered(bool) ),
             this, SLOT( ExtractMIBfromRFC(bool) ) );
    connect( s->MainUI()->MIBLog, SIGNAL ( itemDoubleClicked ( QListWidgetItem* ) ),
             this, SLOT( SelectedLogEntry ( QListWidgetItem* ) ) );
    connect( s->MainUI()->fileNewAction, SIGNAL( triggered(bool) ),
             this, SLOT( MibFileNew(bool) ) );

    // Syntax highlighter
    highlighter = new MibHighlighter(s->MainUI()->MIBFile->document());

    // Marker widget
    s->MainUI()->MIBFileMarker->setTextEditor(s->MainUI()->MIBFile);

    // Line number statusbar widget
    lnum = new QLabel();
    s->MainUI()->MIBFileStatus->addPermanentWidget(lnum);

    connect( s->MainUI()->MIBFile, SIGNAL( cursorPositionChanged() ),
             this, SLOT( SetLineNumStatus() ) );

    SetLineNumStatus();
}

void MibEditor::MibFileNew(bool)
{
    s->MainUI()->MIBFile->clear();
    LoadedFile = "";
}

void MibEditor::MibFileOpen(bool)
{
    QString fileName = NULL;

    fileName = QFileDialog::getOpenFileName(s->MainUI()->MIBFile,
                                tr("Open File"), "", 
                                "MIB Files (*-MIB *-PIB *.mib *.pib *.smi)");

    if (!fileName.isEmpty())
    {
        QFile file(fileName);
        if (file.open(QIODevice::ReadWrite | QFile::Text))
            s->MainUI()->MIBFile->setPlainText(file.readAll());
        LoadedFile = fileName; 
    }
}

void MibEditor::MibFileSave(bool)
{
}

void MibEditor::MibFileSaveAs(bool)
{
}

void MibEditor::ErrorHandler(char *path, int line, int severity, 
                         char *msg, char *tag)
{
    QString message = NULL;
    QListWidgetItem *item;
    QBrush item_brush;
    (void)path;

    switch (severity)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            message += "Error ";
            num_error++;
            item_brush.setColor(Qt::red);
            break;
        case 4:
        case 5:
            message += "Warning ";
            num_warning++;
            item_brush.setColor(Qt::darkYellow);
            break;
        case 6:
        case 7:
        case 8:
        case 9:
            message += "Info ";
            num_info++;
            item_brush.setColor(Qt::blue);
            break;
    }

    message += QString("(level %1), line %2: [%3] %4")
                       .arg(severity).arg(line).arg(tag).arg(msg);
    item = new QListWidgetItem(message, s->MainUI()->MIBLog);
    item->setForeground(item_brush);
    s->MainUI()->MIBLog->addItem(item);
}

MibEditor *CurrentEditorObject = NULL;
static void ErrorHdlr(char *path, int line, int severity, 
                      char *msg, char *tag)
{
    CurrentEditorObject->ErrorHandler(path, line, severity, msg, tag);
}

void MibEditor::VerifyMIB(bool)
{
    int flags =  smiGetFlags();
    int saved_flags = flags;
    flags |= SMI_FLAG_ERRORS;
    flags |= SMI_FLAG_NODESCR;
    smiSetFlags(flags);

    s->MainUI()->MIBLog->clear();
    CurrentEditorObject = this;
    smiSetErrorHandler(ErrorHdlr);
    smiSetErrorLevel(9);

    num_error = 0;
    num_warning = 0;
    num_info = 0;

    QString start_msg = QString("Starting MIB verification ...");
    s->MainUI()->MIBLog->addItem(new QListWidgetItem(start_msg, 
                                                     s->MainUI()->MIBLog));

    smiLoadModule(QDir::toNativeSeparators(LoadedFile).toLatin1().data());

    QString stop_msg = QString("Verification completed. %1 errors, %2 warnings, %3 infos")
                               .arg(num_error).arg(num_warning).arg(num_info);
    s->MainUI()->MIBLog->addItem(new QListWidgetItem(stop_msg, 
                                                     s->MainUI()->MIBLog));

    smiSetFlags(saved_flags);

    // Reload everything
    s->MibModuleObj()->Refresh();
}

void MibEditor::ExtractMIBfromRFC(bool)
{
    QRegExp module_regexp("^[ \t]*([A-Za-z0-9-]*) *(PIB-)?DEFINITIONS *(::=)? *(BEGIN)? *$");
    QRegExp page_regexp("\\[[pP]age [iv0-9]*\\] *");
    QRegExp macro_regexp("^[ \t]*[A-Za-z0-9-]* *MACRO *::=");
    QRegExp end_regexp("^[ \t]*END[ \t]*$");
    QRegExp blankline_regexp("^[ \t]*$");
    QRegExp blank_regexp("[^ \t]");
    QRegExp leadingspaces_regexp("^([ ]*)");
    QRegExp draft_regexp("^[ ]*Internet[ \\-]Draft");

    QFile file_in("empty");
    QFile file_tmpout("empty");
    QFile file_out("empty");
    QTextStream in(&file_in);
    QTextStream tmpout; 
    QTextStream out; 

    QString line;
    QString module;
    QStringList modules;

    QString dir = NULL, filename = NULL;
    int skipmibfile = 0, skip = 0, skipped = 0, macro = 0, n = 0;

    // Open RFC file
    filename = QFileDialog::getOpenFileName(s->MainUI()->MIBFile,
                                        tr("Open RFC file"), "", 
                                        "RFC files (*.txt);;All Files (*.*)");

    if (!filename.isEmpty())
    {
        file_in.setFileName(filename);
        if (!file_in.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox::warning(NULL, tr("SnmpB: Extract MIB from RFC"),
                                 tr("Cannot read file %1: %2\n")
                                 .arg(file_in.fileName())
                                 .arg(file_in.errorString()));
            return;
        }
    }
    else
        return;

    // Ask for directory where to save MIB files 
    dir = QFileDialog::getExistingDirectory(s->MainUI()->MIBFile,
                           tr("Select destination folder for MIB files"), "");

    if (dir.isEmpty())
    {
        QMessageBox::warning(NULL, tr("SnmpB: Extract MIB from RFC"),
                             tr("No directory selected. Aborting.\n"));
        file_in.close();
        return;
    }

    if (!QFileInfo(dir).isWritable())
    {
        QMessageBox::warning(NULL, tr("SnmpB: Extract MIB from RFC"),
                tr("Directory not writable by this user. Aborting.\n"));
        file_in.close();
        return;
    }

    // Extract & save each modules ...

    // Process each line
    while (in.atEnd() != true)
    {
        line = in.readLine();

        if (draft_regexp.indexIn(line) != -1)
            continue;

        // Start of module
        if (module_regexp.indexIn(line) != -1)
        {
            module = module_regexp.cap(1); 
            skip = 9;
            skipped = -1;
            macro = 0;
            n = 0;

            // Create temporary output file
            file_tmpout.setFileName(QDir::tempPath()+"/"+module+".tmp");
            file_tmpout.remove();
            if (!file_tmpout.open(QFile::ReadWrite | QFile::Text))
            {
                QMessageBox::warning(NULL, tr("SnmpB: Extract MIB from RFC"),
                                     tr("Cannot create file %1: %2. Abort.\n")
                                     .arg(file_tmpout.fileName())
                                     .arg(file_tmpout.errorString()));
                file_in.close();
                return;
            }

            tmpout.setDevice(&file_tmpout); 

            // Create output file
            file_out.setFileName(dir+"/"+module);
            if (file_out.exists())
            {
                QMessageBox mb(QMessageBox::Question, 
                               tr("SnmpB: Extract MIB from RFC"), 
                               tr("The file %1 already exist.\n")
                               .arg(file_out.fileName()));
                QPushButton *ob = mb.addButton(tr("Overwrite"), 
                                               QMessageBox::YesRole);
                QPushButton *sb = mb.addButton(tr("Skip"), 
                                               QMessageBox::NoRole);
                mb.exec();

                if (mb.clickedButton() == ob)
                {
                    // overwrite 
                    skipmibfile = 0;
                }
                else if (mb.clickedButton() == sb)
                {
                    // skip 
                    skipmibfile = 1;
                }
            }
            else
                skipmibfile = 0;

            if (!skipmibfile)
            {
                file_out.remove();
                if (!file_out.open(QFile::ReadWrite | QFile::Text))
                {
                    QMessageBox::warning(NULL, tr("SnmpB: Extract MIB from RFC"),
                                         tr("Cannot create file %1: %2. Skipping.\n")
                                         .arg(file_out.fileName())
                                         .arg(file_out.errorString()));
                    skipmibfile = 1;
                }
                else
                    out.setDevice(&file_out);
            }
        }

        // At the end of a page we start the counter skipped to skip the
        // next few lines.
        if (page_regexp.indexIn(line) != -1)
            skipped = 0;

        // If we are skipping...
        if (skipped >= 0)
        {
            skipped++;

            // If we have skipped enough lines to the top of the next page...
            if (skipped >= skip)
            {
                skipped = -1;
            }
            else
            {
                // Finish skipping, if we find a non-empty line, but not before
                // we have skipped four lines. remember the miminum of lines
                // we have ever skipped to keep empty lines in a modules that
                // appear near the top of a page.
                if ((skipped >= 4) && (blank_regexp.indexIn(line) != -1))
                {
                    if (skipped < skip)
                        skip = skipped;

                    skipped = -1;
                }   
            }
        }

        // So, if we are not skipping and inside a module, remember the line.
        if ((skipped == -1) && (module.length() > 0))
        {
            n++;
            tmpout << line << endl;
        }

        // Remember when we enter a macro definition
        if (macro_regexp.indexIn(line) != -1)
            macro = 1;

        // End of module
        if (end_regexp.indexIn(line) != -1)
        {
            if (macro == 0)
            {
                tmpout.flush(); 
                tmpout.seek(0);

                int strip = 99, p = 0;

                while (tmpout.atEnd() != true)
                {
                    line = tmpout.readLine();

                    // Find the minimum column that contains non-blank
                    // characters in order to cut a blank prefix off.
                    // Ignore lines that only contain white spaces.
                    if (blankline_regexp.indexIn(line) == -1)
                    {
                        if (leadingspaces_regexp.indexIn(line) != -1)
                        {
                            p = leadingspaces_regexp.cap(1).length(); 
                            if ((p < strip) && (line.length() > p))
                                strip = p;
                        }
                    }
                }

                tmpout.seek(0);

                if (!skipmibfile)
                {
                    int num_bl = 0;

                    while (tmpout.atEnd() != true)
                    {
                        line = tmpout.readLine();
                        // For each block of consecutive blank lines,
                        // remove all lines but one.
                        if (blankline_regexp.indexIn(line) != -1)
                        {
                            num_bl++;
                            continue;
                        }
                        else
                        {
                            if (num_bl > 0)
                                out << endl;
                            num_bl = 0;
                        }

                        out << line.remove(0, strip) << endl;
                    }

                    out.flush(); 
                    file_out.close();

                    modules << module;
                }

                file_tmpout.remove();

                module = "";
            }
            else
            {
                macro = 0;
            }
        }
    }

    file_in.close();
 
    if(modules.size() > 0)
    {
        QString module_list;
        for (int i = 0; i < modules.size(); i++)
        {
            module_list += "\n\t";
            module_list +=  modules[i];
        }

        QMessageBox::information(NULL, tr("SnmpB: Extract MIB from RFC"),
                                 tr("%1 MIB module(s) have been extracted. \
The following MIB file(s) were created: %2")
                                 .arg(modules.size())
                                 .arg(module_list));
    }
}

void MibEditor::SelectedLogEntry(QListWidgetItem *item)
{
    QRegExp expression(" line ([1-9][0-9]*|0): ");
    expression.indexIn(item->text());
    int line = expression.capturedTexts()[1].toInt();

    s->MainUI()->MIBFileMarker->setMarker(line); 
}

void MibEditor::SetLineNumStatus(void)
{
    QString lc = QString("Line: %1, Col: %2").
                     arg(s->MainUI()->MIBFile->textCursor().blockNumber()+1).
                     arg(s->MainUI()->MIBFile->textCursor().columnNumber()+1);

    lnum->setText(lc);
}