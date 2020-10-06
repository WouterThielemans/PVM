#include "midi.h"
#include "../io/read.h"
#include "../io/endianness.h"
#include "../io/vli.h"

namespace midi {
	void read_chunk_header(std::istream& in, CHUNK_HEADER* head) {
		io::read_to(in, head);
		io::switch_endianness(&head->size);
	}

	std::string header_id(CHUNK_HEADER head) {
		return std::string(head.id, 4);
	}

	void read_mthd(std::istream& in, MTHD* mthhead) {
		io::read_to(in, mthhead);
		io::switch_endianness(&mthhead->header.size);
		io::switch_endianness(&mthhead->type);
		io::switch_endianness(&mthhead->division);
		io::switch_endianness(&mthhead->ntracks);
	}
	
	bool is_meta_event(uint8_t byte) {
		return byte == 0xFF;
	}
	bool is_sysex_event(uint8_t byte) {
		return (byte == 0xF7 || byte == 0xF0);
	}
	bool is_running_status(uint8_t byte) {
		return (byte >> 7) == 0;
	}
	bool is_midi_event(uint8_t byte) {
		uint8_t n = byte >> 4;
		return ((n == 0x08) || (n == 0x09) || (n == 0x0A) || (n == 0x0B) || (n == 0x0C) || (n == 0x0D) || (n == 0x0E));
	}

	uint8_t extract_midi_event_type(uint8_t status) {
		return status >> 4U;
	}

	Channel extract_midi_event_channel(uint8_t status){
		return Channel(status & 0x0F);
	}

	bool is_note_off(uint8_t status) {
		return status == 0x08;
	}
	bool is_note_on(uint8_t status) {
		return status == 0x09;
	}
	bool is_polyphonic_key_pressure(uint8_t status) {
		return status == 0x0A;
	}
	bool is_control_change(uint8_t status) {
		return status == 0x0B;
	}
	bool is_program_change(uint8_t status) {
		return status == 0x0C;
	}
	bool is_channel_pressure(uint8_t status) {
		return status == 0x0D;
	}
	bool is_pitch_wheel_change(uint8_t status) {
		return status == 0x0E;
	}

	void read_mtrk(std::istream& in, EventReceiver& receiver) {
		CHUNK_HEADER header;
		read_chunk_header(in, &header);
		uint8_t previous_identifier;

		bool end_track_reached = false;
		while (!end_track_reached){
			Duration duration(io::read_variable_length_integer(in));
			uint8_t identifier = io::read<uint8_t>(in);
			uint8_t first_data;

			if ((identifier & 0b1000'0000) == 0b0000'0000){
				first_data = identifier;
				identifier = previous_identifier;
			}
			else{
				first_data = io::read<uint8_t>(in);
			}
			if (is_meta_event(identifier)){
				auto length = io::read_variable_length_integer(in);
				auto type = first_data;
				std::unique_ptr<uint8_t[]> data = io::read_array<uint8_t>(in, length);
				receiver.meta(duration, type, std::move(data), length);
				if (type == 0x2F) end_track_reached = true;
			}
			else if (is_sysex_event(identifier)){
				in.putback(first_data);
				auto length = io::read_variable_length_integer(in);
				std::unique_ptr<uint8_t[]> data = io::read_array<uint8_t>(in, length);
				receiver.sysex(duration, std::move(data), length);
			}

			else if (is_midi_event(identifier)){
				auto event_type = extract_midi_event_type(identifier);
				Channel channel(extract_midi_event_channel(identifier));

				if (is_note_off(event_type)){
					NoteNumber note = NoteNumber(first_data);
					auto velocity = io::read<uint8_t>(in);
					receiver.note_off(duration, channel, note, velocity);
				}
				else if(is_note_on(event_type)){
					NoteNumber note = NoteNumber(first_data);
					auto velocity = io::read<uint8_t>(in);
					receiver.note_on(duration, channel, note, velocity);
				}
				else if (is_polyphonic_key_pressure(event_type)){
					NoteNumber note(first_data);
					auto pressure = io::read<uint8_t>(in);
					receiver.polyphonic_key_pressure(duration, channel, note, pressure);
				}
				else if (is_control_change(event_type)){
					auto controller = first_data;
					auto value = io::read<uint8_t>(in);
					receiver.control_change(duration, channel, controller, value);
				}
				else if (is_program_change(event_type)){
					Instrument program(first_data);
					receiver.program_change(duration, channel, program);
				}
				else if (is_channel_pressure(event_type)){
					auto pressure = first_data;
					receiver.channel_pressure(duration, channel, pressure);
				}
				else if (is_pitch_wheel_change(event_type)){
					auto lower_bits = first_data;
					auto upper_bits = io::read<uint8_t>(in);
					auto value = upper_bits << 7 | lower_bits;
					receiver.pitch_wheel_change(duration, channel, value);
				}
			}
			previous_identifier = identifier;
		}
	}


	// ChannelNoteCollector


	void ChannelNoteCollector::note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity){
		if (channel == this->channel){
			if (velocity == 0b00000000){
				note_off(dt, channel, note, velocity);
			}
			else {
				this->start += dt;
				NOTE noot = NOTE();
				noot.note_number = note;
				noot.start = this->start;
				noot.velocity = velocity;
				noot.instrument = this->instrument;
				bool exists = false;
				for (int i = 0; i < this->notes.size(); i++){
					if (this->notes[i].note_number == note){
						note_off(dt, channel, note, velocity);
						exists = true;
						break;
					}
				}
				if (!exists){
					for (int i = 0; i < this->notes.size(); i++){
						this->notes[i].duration += dt;
					}
				}
				this->notes.push_back(noot);
			}
		}
		else {
			action_other_channel(dt);
		}
	}

	void ChannelNoteCollector::note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity){
		if (channel == this->channel){
			int index = -1;
			for (int i = 0; i < this->notes.size(); i++){
				this->notes[i].duration += dt;
				if (this->notes[i].note_number == note) {
					NOTE noot = this->notes[i];
					this->note_receiver(noot);
					index = i;
				}
			}
			if (index != -1){
				this->notes.erase(this->notes.begin() + index);
			}
			this->start += dt;
		}
		else {
			action_other_channel(dt);
		}
	}

	void ChannelNoteCollector::polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure){
		if (channel == this->channel){
			for (int i = 0; i < this->notes.size(); i++){
				this->notes[i].duration += dt;
			}
			this->start += dt;
		}
		else {
			action_other_channel(dt);
		}
	}

	void ChannelNoteCollector::control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value){
		if (channel == this->channel){
			for (int i = 0; i < this->notes.size(); i++)
			{
				this->notes[i].duration += dt;
			}
			this->start += dt;
		}
		else {
			action_other_channel(dt);
		}
	}

	void ChannelNoteCollector::program_change(Duration dt, Channel channel, Instrument program){
		if (channel == this->channel){
			this->instrument = program;
			for (int i = 0; i < this->notes.size(); i++){
				this->notes[i].duration += dt;
			}
			this->start += dt;
		}
		else {
			action_other_channel(dt);
		}
	}

	void ChannelNoteCollector::channel_pressure(Duration dt, Channel channel, uint8_t pressure){
		if (channel == this->channel){
			for (int i = 0; i < this->notes.size(); i++){
				this->notes[i].duration += dt;
			}
			this->start += dt;
		}
		else {
			action_other_channel(dt);
		}
	}

	void ChannelNoteCollector::pitch_wheel_change(Duration dt, Channel channel, uint16_t value){
		if (channel == this->channel){
			for (int i = 0; i < this->notes.size(); i++){
				this->notes[i].duration += dt;
			}
			this->start += dt;
		}
		else {
			action_other_channel(dt);
		}
	}

	void ChannelNoteCollector::meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size){
		for (int i = 0; i < this->notes.size(); i++){
			this->notes[i].duration += dt;
		}
		this->start += dt;
	}

	void ChannelNoteCollector::sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size){
		for (int i = 0; i < this->notes.size(); i++){
			this->notes[i].duration += dt;
		}
		this->start += dt;
	}

	void ChannelNoteCollector::action_other_channel(Duration dt){
		for (int i = 0; i < this->notes.size(); i++){
			this->notes[i].duration += dt;
		}
		this->start += dt;
	}

	std::ostream&operator<<(std::ostream& out, const NOTE& note) {
		return out << "Note(number=" << note.note_number
			<< ",start=" << note.start
			<< ",duration=" << note.duration
			<< ",instrument=" << note.instrument
			<< ")";
	}

	bool operator==(const NOTE& x, const NOTE& y) {
		return x.duration == y.duration && x.instrument == y.instrument && x.note_number == y.note_number && x.start == y.start && x.velocity == y.velocity;
	}

	bool operator!=(const NOTE& x, const NOTE& y) {
		return !(x == y);
	}


	// EventMultiCaster 
	

	void EventMulticaster::note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity){
		for (const auto& receiver : receivers){
			receiver->note_on(dt, channel, note, velocity);
		}
	}
	void EventMulticaster::note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity){
		for (const auto& event_receiver : receivers){
			event_receiver->note_off(dt, channel, note, velocity);
		}
	}
	void EventMulticaster::polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure){
		for (const auto& event_receiver : receivers){
			event_receiver->polyphonic_key_pressure(dt, channel, note, pressure);
		}
	}
	void EventMulticaster::control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value){
		for (const auto& event_receiver : receivers){
			event_receiver->control_change(dt, channel, controller, value);
		}
	}
	void EventMulticaster::program_change(Duration dt, Channel channel, Instrument program){
		for (const auto& event_receiver : receivers){
			event_receiver->program_change(dt, channel, program);
		}
	}
	void EventMulticaster::channel_pressure(Duration dt, Channel channel, uint8_t pressure){
		for (const auto& event_receiver : receivers){
			event_receiver->channel_pressure(dt, channel, pressure);
		}
	}
	void EventMulticaster::pitch_wheel_change(Duration dt, Channel channel, uint16_t value){
		for (const auto& event_receiver : receivers){
			event_receiver->pitch_wheel_change(dt, channel, value);
		}
	}
	void EventMulticaster::meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size){
		for (const auto& event_receiver : receivers){
			event_receiver->meta(dt, type, std::move(data), data_size);
		}
	}
	void EventMulticaster::sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size){
		for (const auto& event_receiver : receivers){
			event_receiver->sysex(dt, std::move(data), data_size);
		}
	}
	void EventMulticaster::add_event_receiver(const std::shared_ptr<EventReceiver>& eventReceiver){
		if (receivers.size() < 16){
			receivers.push_back(eventReceiver);
		}
	}

	
	// NoteCollector 


	void NoteCollector::note_on(Duration dt,Channel channel, NoteNumber note, uint8_t velocity){
		this->multicaster.note_on(dt, channel, note, velocity);
	}
	void NoteCollector::note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity){
		this->multicaster.note_off(dt, channel, note, velocity);
	}
	void NoteCollector::polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure){
		this->multicaster.polyphonic_key_pressure(dt, channel, note, pressure);
	}
	void NoteCollector::control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value){
		this->multicaster.control_change(dt, channel, controller, value);
	}
	void NoteCollector::program_change(Duration dt, Channel channel, Instrument program){
		this->multicaster.program_change(dt, channel, program);
	}
	void NoteCollector::channel_pressure(Duration dt, Channel channel, uint8_t pressure){
		this->multicaster.channel_pressure(dt, channel, pressure);
	}
	void NoteCollector::pitch_wheel_change(Duration dt, Channel channel, uint16_t value){
		this->multicaster.pitch_wheel_change(dt, channel, value);
	}
	void NoteCollector::meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size){
		this->multicaster.meta(dt, type, std::move(data), data_size);
	}
	void NoteCollector::sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size){
		this->multicaster.sysex(dt, std::move(data), data_size);
	}

	std::vector<NOTE> read_notes(std::istream& in){
		MTHD mthhead;
		read_mthd(in, &mthhead);
		std::vector<NOTE> notes;

		for (int i = 0; i < mthhead.ntracks; i++){
			NoteCollector collector = NoteCollector([&notes](const NOTE& note)
			{ notes.push_back(note); });
			read_mtrk(in, collector);
		}
		return notes;
	}
}