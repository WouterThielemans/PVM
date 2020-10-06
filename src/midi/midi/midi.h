#pragma once
#ifndef MIDI_H
#define MIDI_H

#include <sstream>
#include <istream>
#include "primitives.h"
#include <functional>
#include <vector>
#include <memory>

namespace midi {
	struct CHUNK_HEADER {
	public:
		char id[4];
		uint32_t size;
	};
	void read_chunk_header(std::istream&, CHUNK_HEADER*);

	std::string header_id(CHUNK_HEADER);

#pragma pack(push, 1)
	struct MTHD {
		CHUNK_HEADER header;
		uint16_t type;
		uint16_t ntracks;
		uint16_t division;
	};
#pragma pack(pop)
	void read_mthd(std::istream&, MTHD*);
	bool is_sysex_event(uint8_t);
	bool is_meta_event(uint8_t);
	bool is_midi_event(uint8_t);
	bool is_running_status(uint8_t);
	uint8_t extract_midi_event_type(uint8_t);
	Channel extract_midi_event_channel(uint8_t);
	bool is_note_off(uint8_t);
	bool is_note_on(uint8_t);
	bool is_polyphonic_key_pressure(uint8_t);
	bool is_control_change(uint8_t);
	bool is_program_change(uint8_t);
	bool is_channel_pressure(uint8_t);
	bool is_pitch_wheel_change(uint8_t);

	class EventReceiver{
	public:
		virtual void note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) = 0;
		virtual void note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) = 0;
		virtual void polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure) = 0;
		virtual void control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value) = 0;
		virtual void program_change(Duration dt, Channel channel, Instrument program) = 0;
		virtual void channel_pressure(Duration dt, Channel channel, uint8_t pressure) = 0;
		virtual void pitch_wheel_change(Duration dt, Channel channel, uint16_t value) = 0;
		virtual void meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size) = 0;
		virtual void sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size) = 0;
	};

	void read_mtrk(std::istream&, EventReceiver&);

	//ChannelNoteCollector

	struct NOTE{
		NoteNumber note_number;
		Time start;
		Duration duration;
		uint8_t velocity;
		Instrument instrument;

		NOTE(NoteNumber note_number, Time start, Duration duration, uint8_t velocity, Instrument instrument) : note_number(note_number), start(start), duration(duration), velocity(velocity), instrument(instrument) {};
		NOTE() : note_number(0), start(0), duration(0), velocity(0), instrument(0) {};
	};

	std::ostream& operator <<(std::ostream& out, const NOTE& note);
	bool operator ==(const NOTE& x, const NOTE& y);
	bool operator !=(const NOTE& x, const NOTE& y);

	class ChannelNoteCollector : public EventReceiver{
		Channel channel;
		std::function<void(const NOTE&)> note_receiver;
		std::vector<NOTE> notes;
		Time start;
		Instrument instrument;

	public:
		ChannelNoteCollector(Channel channel, std::function<void(const NOTE&)> note_receiver) : channel(channel), note_receiver(note_receiver), start(0), notes(0), instrument(0) {};
		void note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) override;
		void note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) override;
		void polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure) override;
		void control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value) override;
		void program_change(Duration dt, Channel channel, Instrument program) override;
		void channel_pressure(Duration dt, Channel channel, uint8_t pressure) override;
		void pitch_wheel_change(Duration dt, Channel channel, uint16_t value) override;
		void meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size) override;
		void sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size) override;
		void action_other_channel(Duration dt);
	};


	//EventMulticaster


	struct EventMulticaster : EventReceiver{
	public:
		std::vector<std::shared_ptr<EventReceiver>> receivers;
		explicit EventMulticaster(std::vector<std::shared_ptr<EventReceiver>> receivers) : receivers(std::move(receivers)) {};

		void note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) override;
		void note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) override;
		void polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure) override;
		void control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value) override;
		void program_change(Duration dt, Channel channel, Instrument program) override;
		void channel_pressure(Duration dt, Channel channel, uint8_t pressure) override;
		void pitch_wheel_change(Duration dt, Channel channel, uint16_t value) override;
		void meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size) override;
		void sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size) override;
		void add_event_receiver(const std::shared_ptr<EventReceiver>&);
	};


	//NoteCollector


	class NoteCollector : public EventReceiver{
	public:
		std::function<void(const NOTE&)> receiver;
		EventMulticaster multicaster;

		static std::vector<std::shared_ptr<EventReceiver >> create_list(std::function<void(const NOTE&)> receiver){
			std::vector<std::shared_ptr<EventReceiver>> receivers;
			for (int channel = 0; channel < 16; channel++){
				auto shptr = std::make_shared<ChannelNoteCollector>(Channel(channel), receiver);
				receivers.push_back(shptr);
			}
			return receivers;
		}

		NoteCollector(std::function<void(const NOTE&)> receiver) : receiver(receiver), multicaster(create_list(receiver)) { }

		void note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) override;
		void note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) override;
		void polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure) override;
		void control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value) override;
		void program_change(Duration dt, Channel channel, Instrument program) override;
		void channel_pressure(Duration dt, Channel channel, uint8_t pressure) override;
		void pitch_wheel_change(Duration dt, Channel channel, uint16_t value) override;
		void meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size) override;
		void sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size) override;
	};
	std::vector<NOTE> read_notes(std::istream&);
}
#endif