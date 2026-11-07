#include <QBuffer>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QPainter>
#include <QSvgRenderer>
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    if (argc != 2)
        return 1;
    QDir dir(argv[1]);
    QImage board(480, 540, QImage::Format_ARGB32_Premultiplied);
    board.fill(QColor("#10151d"));
    QPainter b(&board);
    int index = 0;
    for (auto name : {"infinity"}) {
        QSvgRenderer svg(dir.filePath(QString(name) + ".svg"));
        QImage img(1024, 1024, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
        {
            QPainter p(&img);
            svg.render(&p);
        }
        img.save(dir.filePath(QString(name) + ".png"));
        b.drawImage(QRect(index * 480 + 48, 28, 384, 384), img);
        b.setPen(QColor("#cdd9e5"));
        QFont f;
        f.setPixelSize(22);
        b.setFont(f);
        b.drawText(QRect(index * 480, 440, 480, 40), Qt::AlignCenter, name);
        index++;
    }
    b.end();
    board.save(dir.filePath("infinity-concept.png"));
    QImage source(dir.filePath("infinity.png"));
    dir.mkpath("Infinity.iconset");
    for (int size : {16, 32, 128, 256, 512}) {
        source.scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
            .save(dir.filePath(QString("Infinity.iconset/icon_%1x%1.png").arg(size)));
        source.scaled(size * 2, size * 2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
            .save(dir.filePath(QString("Infinity.iconset/icon_%1x%1@2x.png").arg(size)));
    }
    QFile ico(dir.filePath("Infinity.ico"));
    if (!ico.open(QIODevice::WriteOnly))
        return 2;
    QDataStream out(&ico);
    out.setByteOrder(QDataStream::LittleEndian);
    out << quint16(0) << quint16(1) << quint16(6);
    QList<QByteArray> images;
    for (int size : {16, 32, 48, 64, 128, 256}) {
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        source.scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
            .save(&buffer, "PNG");
        images << bytes;
    }
    quint32 offset = 6 + 16 * 6;
    int i = 0;
    for (int size : {16, 32, 48, 64, 128, 256}) {
        out << quint8(size == 256 ? 0 : size) << quint8(size == 256 ? 0 : size) << quint8(0)
            << quint8(0) << quint16(1) << quint16(32) << quint32(images[i].size()) << offset;
        offset += images[i++].size();
    }
    for (auto bytes : images)
        ico.write(bytes);
    return 0;
}
